
#pragma once
#include <string>

struct BucketStat {
    int remaining;
    long long reset_after_ms;
};

struct ClientConfig {
    int capacity;
    int refill_rate_per_sec;
    int ttl_seconds;
};

struct QuotaUpdate {
    enum class Mode {
        kAddTokens,
        kSetTokens,
    };

    Mode mode;
    int value;
    bool reset_refill_time;
};

class ITokenStore {
public:
    virtual ~ITokenStore() = default;
    virtual bool consume(const std::string& client_id,
                         int tokens,
                         BucketStat& stat,
                         ClientConfig* applied_config) = 0;
    virtual ClientConfig getClientConfig(const std::string& client_id) = 0;
    virtual void updateQuota(const std::string& client_id,
                             const QuotaUpdate& update,
                             BucketStat& stat,
                             ClientConfig& applied_config) = 0;
    virtual void upsertClientConfig(const std::string& client_id,
                                    const ClientConfig& config,
                                    bool reset_quota_state,
                                    BucketStat& stat,
                                    ClientConfig& applied_config) = 0;
};
