
#include "redis_store.h"
#include <iostream>

static const char* LUA_SCRIPT =
"local key=KEYS[1] "
.."local capacity=tonumber(ARGV[1]) "
.."local refill=tonumber(ARGV[2]) "
.."local tokens=tonumber(ARGV[3]) "
.."local now=redis.call('TIME')[1] "
.."local bucket=redis.call('HMGET',key,'tokens','ts') "
.."local current_tokens, ts = tonumber(bucket[1]), tonumber(bucket[2]) "
.."if not current_tokens then current_tokens=capacity ts=now end "
.."local elapsed = now - ts "
.."local new_tokens = math.min(capacity, current_tokens + elapsed*refill) "
.."if new_tokens < tokens then "
.."redis.call('HMSET',key,'tokens',new_tokens,'ts',now) "
.."return {0,new_tokens, (capacity-new_tokens)/refill} "
.."else "
.."redis.call('HMSET',key,'tokens',new_tokens - tokens,'ts',now) "
.."return {1,new_tokens - tokens, (capacity-new_tokens+tokens)/refill} "
.."end";

RedisStore::RedisStore(const std::string& host,int port,int capacity,int refill)
    :capacity_(capacity),refill_(refill){
    ctx_=redisConnect(host.c_str(),port);
    if(ctx_==nullptr || ctx_->err){
        throw std::runtime_error("Redis connection failed");
    }
    loadLuaScript();
}
RedisStore::~RedisStore(){
    if(ctx_) redisFree(ctx_);
}

void RedisStore::loadLuaScript(){
    redisReply *reply=(redisReply*)redisCommand(ctx_,"SCRIPT LOAD %s",LUA_SCRIPT);
    if(reply==nullptr) throw std::runtime_error("Lua load failed");
    lua_sha_=reply->str;
    freeReplyObject(reply);
}

bool RedisStore::consume(const std::string& key,int tokens,BucketStat &stat){
    std::lock_guard<std::mutex> lock(mtx_);
    redisReply* reply=(redisReply*)redisCommand(ctx_,"EVALSHA %s 1 %s %d %d %d",lua_sha_.c_str(),key.c_str(),capacity_,refill_,tokens);
    if(!reply) throw std::runtime_error("Redis eval failed");
    if(reply->type!=REDIS_REPLY_ARRAY || reply->elements<3){
        freeReplyObject(reply);
        throw std::runtime_error("Unexpected reply");
    }
    bool allowed = reply->element[0]->integer==1;
    stat.remaining=reply->element[1]->integer;
    stat.reset_after_ms=reply->element[2]->integer*1000;
    freeReplyObject(reply);
    return allowed;
}
