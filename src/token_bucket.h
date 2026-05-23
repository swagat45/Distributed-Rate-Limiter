#pragma once

#include "backend_interface.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

struct TokenBucketState {
    double tokens;
    long long last_refill_ms;
};

class TokenBucketMath {
public:
    static void validateConfig(const ClientConfig& config) {
        if (config.capacity <= 0) {
            throw std::invalid_argument("capacity must be positive");
        }
        if (config.refill_rate_per_sec <= 0) {
            throw std::invalid_argument("refill_rate_per_sec must be positive");
        }
        if (config.ttl_seconds <= 0) {
            throw std::invalid_argument("ttl_seconds must be positive");
        }
    }

    static long long nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    static TokenBucketState refill(const TokenBucketState& state,
                                   const ClientConfig& config,
                                   long long now_ms) {
        validateConfig(config);

        TokenBucketState updated = state;
        if (updated.last_refill_ms <= 0) {
            updated.last_refill_ms = now_ms;
        }

        const long long elapsed_ms = std::max(0LL, now_ms - updated.last_refill_ms);
        const double refilled_tokens =
            (static_cast<double>(elapsed_ms) * config.refill_rate_per_sec) / 1000.0;

        updated.tokens = std::clamp(updated.tokens + refilled_tokens,
                                    0.0,
                                    static_cast<double>(config.capacity));
        updated.last_refill_ms = now_ms;
        return updated;
    }

    static long long timeToAvailabilityMs(double available_tokens,
                                          int requested_tokens,
                                          const ClientConfig& config) {
        validateConfig(config);
        if (available_tokens >= requested_tokens) {
            return 0;
        }

        const double missing_tokens = requested_tokens - available_tokens;
        return static_cast<long long>(std::ceil(
            (missing_tokens * 1000.0) / config.refill_rate_per_sec));
    }

    static long long timeToFullMs(double available_tokens, const ClientConfig& config) {
        validateConfig(config);
        if (available_tokens >= config.capacity) {
            return 0;
        }

        const double missing_tokens = config.capacity - available_tokens;
        return static_cast<long long>(std::ceil(
            (missing_tokens * 1000.0) / config.refill_rate_per_sec));
    }

    static bool consume(TokenBucketState& state,
                        const ClientConfig& config,
                        int tokens,
                        long long now_ms,
                        BucketStat& stat) {
        validateConfig(config);
        if (tokens <= 0) {
            throw std::invalid_argument("tokens must be positive");
        }

        state = refill(state, config, now_ms);
        if (state.tokens >= tokens) {
            state.tokens -= tokens;
            stat.remaining = static_cast<int>(std::floor(state.tokens));
            stat.reset_after_ms = timeToFullMs(state.tokens, config);
            return true;
        }

        stat.remaining = static_cast<int>(std::floor(state.tokens));
        stat.reset_after_ms = timeToAvailabilityMs(state.tokens, tokens, config);
        return false;
    }

    static void applyQuotaUpdate(TokenBucketState& state,
                                 const ClientConfig& config,
                                 const QuotaUpdate& update,
                                 long long now_ms,
                                 BucketStat& stat) {
        validateConfig(config);

        state = refill(state, config, now_ms);

        if (update.mode == QuotaUpdate::Mode::kAddTokens) {
            state.tokens += update.value;
        } else {
            state.tokens = update.value;
        }

        state.tokens = std::clamp(state.tokens, 0.0, static_cast<double>(config.capacity));
        if (update.reset_refill_time) {
            state.last_refill_ms = now_ms;
        }

        stat.remaining = static_cast<int>(std::floor(state.tokens));
        stat.reset_after_ms = timeToFullMs(state.tokens, config);
    }

    static TokenBucketState reconcileConfig(TokenBucketState state,
                                            const ClientConfig& config,
                                            bool reset_quota_state,
                                            long long now_ms,
                                            BucketStat& stat) {
        validateConfig(config);

        if (reset_quota_state) {
            state.tokens = config.capacity;
            state.last_refill_ms = now_ms;
        } else {
            state = refill(state, config, now_ms);
            state.tokens = std::clamp(state.tokens, 0.0, static_cast<double>(config.capacity));
        }

        stat.remaining = static_cast<int>(std::floor(state.tokens));
        stat.reset_after_ms = timeToFullMs(state.tokens, config);
        return state;
    }
};

class TokenBucket {
public:
    TokenBucket(int capacity, int refill_rate)
        : config_{capacity, refill_rate, 60},
          state_{static_cast<double>(capacity), TokenBucketMath::nowMs()} {}

    bool consume(int tokens, int& remaining, long long& reset_after_ms) {
        BucketStat stat{};
        const bool allowed =
            TokenBucketMath::consume(state_, config_, tokens, TokenBucketMath::nowMs(), stat);
        remaining = stat.remaining;
        reset_after_ms = stat.reset_after_ms;
        return allowed;
    }

private:
    ClientConfig config_;
    TokenBucketState state_;
};
