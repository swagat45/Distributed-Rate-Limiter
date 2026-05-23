#include "redis_store.h"
#include "token_bucket.h"
#include <algorithm>
#include <stdexcept>

namespace {

const char* kConsumeScript =
    "local bucket_key = KEYS[1] "
    "local capacity = tonumber(ARGV[1]) "
    "local refill = tonumber(ARGV[2]) "
    "local requested = tonumber(ARGV[3]) "
    "local ttl_ms = tonumber(ARGV[4]) "
    "local now_ms = tonumber(ARGV[5]) "
    "local bucket = redis.call('HMGET', bucket_key, 'tokens', 'ts') "
    "local current_tokens = tonumber(bucket[1]) "
    "local last_refill_ms = tonumber(bucket[2]) "
    "if not current_tokens then current_tokens = capacity last_refill_ms = now_ms end "
    "local elapsed_ms = math.max(0, now_ms - last_refill_ms) "
    "local refilled = (elapsed_ms * refill) / 1000.0 "
    "local available = math.min(capacity, current_tokens + refilled) "
    "local allowed = 0 "
    "local remaining = available "
    "local reset_after_ms = 0 "
    "if available >= requested then "
    "  allowed = 1 "
    "  remaining = available - requested "
    "  if remaining < capacity then "
    "    reset_after_ms = math.ceil(((capacity - remaining) * 1000.0) / refill) "
    "  end "
    "else "
    "  reset_after_ms = math.ceil(((requested - available) * 1000.0) / refill) "
    "end "
    "redis.call('HMSET', bucket_key, 'tokens', remaining, 'ts', now_ms) "
    "redis.call('PEXPIRE', bucket_key, ttl_ms) "
    "return {allowed, math.floor(remaining), reset_after_ms}";

const char* kUpdateQuotaScript =
    "local bucket_key = KEYS[1] "
    "local capacity = tonumber(ARGV[1]) "
    "local refill = tonumber(ARGV[2]) "
    "local ttl_ms = tonumber(ARGV[3]) "
    "local now_ms = tonumber(ARGV[4]) "
    "local mode = ARGV[5] "
    "local amount = tonumber(ARGV[6]) "
    "local reset_refill = tonumber(ARGV[7]) "
    "local bucket = redis.call('HMGET', bucket_key, 'tokens', 'ts') "
    "local current_tokens = tonumber(bucket[1]) "
    "local last_refill_ms = tonumber(bucket[2]) "
    "if not current_tokens then current_tokens = capacity last_refill_ms = now_ms end "
    "local elapsed_ms = math.max(0, now_ms - last_refill_ms) "
    "local refilled = (elapsed_ms * refill) / 1000.0 "
    "local available = math.min(capacity, current_tokens + refilled) "
    "local remaining = available "
    "if mode == 'delta' then "
    "  remaining = available + amount "
    "else "
    "  remaining = amount "
    "end "
    "if remaining < 0 then remaining = 0 end "
    "if remaining > capacity then remaining = capacity end "
    "if reset_refill == 1 then last_refill_ms = now_ms else last_refill_ms = now_ms end "
    "local reset_after_ms = 0 "
    "if remaining < capacity then "
    "  reset_after_ms = math.ceil(((capacity - remaining) * 1000.0) / refill) "
    "end "
    "redis.call('HMSET', bucket_key, 'tokens', remaining, 'ts', last_refill_ms) "
    "redis.call('PEXPIRE', bucket_key, ttl_ms) "
    "return {math.floor(remaining), reset_after_ms}";

const char* kReconcileConfigScript =
    "local bucket_key = KEYS[1] "
    "local capacity = tonumber(ARGV[1]) "
    "local refill = tonumber(ARGV[2]) "
    "local ttl_ms = tonumber(ARGV[3]) "
    "local now_ms = tonumber(ARGV[4]) "
    "local reset_quota = tonumber(ARGV[5]) "
    "local bucket = redis.call('HMGET', bucket_key, 'tokens', 'ts') "
    "local current_tokens = tonumber(bucket[1]) "
    "local last_refill_ms = tonumber(bucket[2]) "
    "if not current_tokens then current_tokens = capacity last_refill_ms = now_ms end "
    "if reset_quota == 1 then "
    "  current_tokens = capacity "
    "  last_refill_ms = now_ms "
    "else "
    "  local elapsed_ms = math.max(0, now_ms - last_refill_ms) "
    "  local refilled = (elapsed_ms * refill) / 1000.0 "
    "  current_tokens = math.min(capacity, current_tokens + refilled) "
    "  if current_tokens < 0 then current_tokens = 0 end "
    "  last_refill_ms = now_ms "
    "end "
    "local reset_after_ms = 0 "
    "if current_tokens < capacity then "
    "  reset_after_ms = math.ceil(((capacity - current_tokens) * 1000.0) / refill) "
    "end "
    "redis.call('HMSET', bucket_key, 'tokens', current_tokens, 'ts', last_refill_ms) "
    "redis.call('PEXPIRE', bucket_key, ttl_ms) "
    "return {math.floor(current_tokens), reset_after_ms}";

void ensureOk(redisContext* ctx, redisReply* reply, const char* action) {
    if (reply == nullptr) {
        throw std::runtime_error(std::string(action) + ": Redis connection error");
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        const std::string message = reply->str ? reply->str : "unknown Redis error";
        freeReplyObject(reply);
        throw std::runtime_error(std::string(action) + ": " + message);
    }
}

std::string loadScript(redisContext* ctx, const char* script) {
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx, "SCRIPT LOAD %s", script));
    ensureOk(ctx, reply, "Lua script load failed");
    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        throw std::runtime_error("Lua script load failed: unexpected reply type");
    }
    const std::string sha = reply->str;
    freeReplyObject(reply);
    return sha;
}

long long currentTimeMs() {
    return TokenBucketMath::nowMs();
}

}  // namespace

RedisStore::RedisStore(const std::string& host, int port, const ClientConfig& default_config)
    : ctx_(nullptr), default_config_(normalizeConfig(default_config, default_config)) {
    struct timeval timeout = {1, 500000};
    ctx_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    if (ctx_ == nullptr || ctx_->err) {
        const std::string error_message =
            ctx_ ? std::string(ctx_->errstr) : "connection context is null";
        if (ctx_ != nullptr) {
            redisFree(ctx_);
        }
        throw std::runtime_error("Redis connection failed: " + error_message);
    }

    loadLuaScripts();
}

RedisStore::~RedisStore() {
    if (ctx_ != nullptr) {
        redisFree(ctx_);
    }
}

bool RedisStore::consume(const std::string& client_id,
                         int tokens,
                         BucketStat& stat,
                         ClientConfig* applied_config) {
    if (tokens <= 0) {
        throw std::invalid_argument("tokens must be positive");
    }

    std::lock_guard<std::mutex> lock(mtx_);
    const ClientConfig config = loadClientConfigUnlocked(client_id);
    const int ttl_ms = config.ttl_seconds * 1000;
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_,
                     "EVALSHA %s 1 %s %d %d %d %d %lld",
                     consume_sha_.c_str(),
                     bucketKey(client_id).c_str(),
                     config.capacity,
                     config.refill_rate_per_sec,
                     tokens,
                     ttl_ms,
                     currentTimeMs()));

    ensureOk(ctx_, reply, "Quota consume failed");
    if (reply->type != REDIS_REPLY_ARRAY || reply->elements < 3) {
        freeReplyObject(reply);
        throw std::runtime_error("Quota consume failed: unexpected reply format");
    }

    const bool allowed = reply->element[0]->integer == 1;
    stat.remaining = static_cast<int>(reply->element[1]->integer);
    stat.reset_after_ms = reply->element[2]->integer;
    freeReplyObject(reply);

    if (applied_config != nullptr) {
        *applied_config = config;
    }

    return allowed;
}

ClientConfig RedisStore::getClientConfig(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    return loadClientConfigUnlocked(client_id);
}

void RedisStore::updateQuota(const std::string& client_id,
                             const QuotaUpdate& update,
                             BucketStat& stat,
                             ClientConfig& applied_config) {
    std::lock_guard<std::mutex> lock(mtx_);
    applied_config = loadClientConfigUnlocked(client_id);

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_,
                     "EVALSHA %s 1 %s %d %d %d %lld %s %d %d",
                     update_sha_.c_str(),
                     bucketKey(client_id).c_str(),
                     applied_config.capacity,
                     applied_config.refill_rate_per_sec,
                     applied_config.ttl_seconds * 1000,
                     currentTimeMs(),
                     update.mode == QuotaUpdate::Mode::kAddTokens ? "delta" : "set",
                     update.value,
                     update.reset_refill_time ? 1 : 0));

    ensureOk(ctx_, reply, "Quota update failed");
    if (reply->type != REDIS_REPLY_ARRAY || reply->elements < 2) {
        freeReplyObject(reply);
        throw std::runtime_error("Quota update failed: unexpected reply format");
    }

    stat.remaining = static_cast<int>(reply->element[0]->integer);
    stat.reset_after_ms = reply->element[1]->integer;
    freeReplyObject(reply);
}

void RedisStore::upsertClientConfig(const std::string& client_id,
                                    const ClientConfig& config,
                                    bool reset_quota_state,
                                    BucketStat& stat,
                                    ClientConfig& applied_config) {
    applied_config = normalizeConfig(config, default_config_);

    std::lock_guard<std::mutex> lock(mtx_);
    redisReply* config_reply = static_cast<redisReply*>(
        redisCommand(ctx_,
                     "HMSET %s capacity %d refill_rate_per_sec %d ttl_seconds %d",
                     configKey(client_id).c_str(),
                     applied_config.capacity,
                     applied_config.refill_rate_per_sec,
                     applied_config.ttl_seconds));
    ensureOk(ctx_, config_reply, "Config write failed");
    freeReplyObject(config_reply);

    redisReply* expire_reply = static_cast<redisReply*>(
        redisCommand(ctx_,
                     "PEXPIRE %s %d",
                     configKey(client_id).c_str(),
                     applied_config.ttl_seconds * 1000));
    ensureOk(ctx_, expire_reply, "Config expiry update failed");
    freeReplyObject(expire_reply);

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_,
                     "EVALSHA %s 1 %s %d %d %d %lld %d",
                     reconcile_sha_.c_str(),
                     bucketKey(client_id).c_str(),
                     applied_config.capacity,
                     applied_config.refill_rate_per_sec,
                     applied_config.ttl_seconds * 1000,
                     currentTimeMs(),
                     reset_quota_state ? 1 : 0));

    ensureOk(ctx_, reply, "Config reconciliation failed");
    if (reply->type != REDIS_REPLY_ARRAY || reply->elements < 2) {
        freeReplyObject(reply);
        throw std::runtime_error("Config reconciliation failed: unexpected reply format");
    }

    stat.remaining = static_cast<int>(reply->element[0]->integer);
    stat.reset_after_ms = reply->element[1]->integer;
    freeReplyObject(reply);
}

void RedisStore::loadLuaScripts() {
    consume_sha_ = loadScript(ctx_, kConsumeScript);
    update_sha_ = loadScript(ctx_, kUpdateQuotaScript);
    reconcile_sha_ = loadScript(ctx_, kReconcileConfigScript);
}

ClientConfig RedisStore::loadClientConfigUnlocked(const std::string& client_id) {
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_,
                     "HMGET %s capacity refill_rate_per_sec ttl_seconds",
                     configKey(client_id).c_str()));
    ensureOk(ctx_, reply, "Config read failed");
    if (reply->type != REDIS_REPLY_ARRAY || reply->elements < 3) {
        freeReplyObject(reply);
        throw std::runtime_error("Config read failed: unexpected reply format");
    }

    ClientConfig loaded = default_config_;
    if (reply->element[0]->type != REDIS_REPLY_NIL) {
        loaded.capacity = std::stoi(reply->element[0]->str);
    }
    if (reply->element[1]->type != REDIS_REPLY_NIL) {
        loaded.refill_rate_per_sec = std::stoi(reply->element[1]->str);
    }
    if (reply->element[2]->type != REDIS_REPLY_NIL) {
        loaded.ttl_seconds = std::stoi(reply->element[2]->str);
    }

    freeReplyObject(reply);
    return normalizeConfig(loaded, default_config_);
}

ClientConfig RedisStore::normalizeConfig(const ClientConfig& config,
                                         const ClientConfig& fallback) {
    ClientConfig normalized = config;
    if (normalized.capacity <= 0) {
        normalized.capacity = fallback.capacity;
    }
    if (normalized.refill_rate_per_sec <= 0) {
        normalized.refill_rate_per_sec = fallback.refill_rate_per_sec;
    }
    if (normalized.ttl_seconds <= 0) {
        normalized.ttl_seconds = fallback.ttl_seconds;
    }
    TokenBucketMath::validateConfig(normalized);
    return normalized;
}

std::string RedisStore::configKey(const std::string& client_id) {
    return "rl:cfg:" + client_id;
}

std::string RedisStore::bucketKey(const std::string& client_id) {
    return "rl:bkt:" + client_id;
}
