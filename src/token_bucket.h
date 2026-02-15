#pragma once
#include <chrono>
#include <cmath>

class TokenBucket {
public:
    TokenBucket(int capacity, int refill_rate)
        : capacity_(capacity), refill_rate_(refill_rate),
          current_tokens_(capacity),
          last_refill_time_(std::chrono::steady_clock::now()) {}

    bool consume(int tokens, int& remaining, long long& reset_after_ms) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_refill_time_).count();

        // Refill tokens based on elapsed time
        double refilled = (elapsed / 1000.0) * refill_rate_;
        current_tokens_ = std::min(capacity_, static_cast<int>(current_tokens_ + refilled));
        last_refill_time_ = now;

        remaining = current_tokens_;

        if (current_tokens_ >= tokens) {
            current_tokens_ -= tokens;
            remaining = current_tokens_;
            // Time until bucket is full again
            reset_after_ms = static_cast<long long>(
                (capacity_ - current_tokens_) * 1000.0 / refill_rate_);
            return true;
        } else {
            // Time until enough tokens are available
            reset_after_ms = static_cast<long long>(
                (tokens - current_tokens_) * 1000.0 / refill_rate_);
            return false;
        }
    }

private:
    int capacity_;
    int refill_rate_;
    double current_tokens_;
    std::chrono::steady_clock::time_point last_refill_time_;
};
