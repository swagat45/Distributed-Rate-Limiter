#pragma once

#include "backend_interface.h"
#include <hiredis/hiredis.h>
#include <mutex>
#include <string>

class RedisStore : public ITokenStore {
public:
    RedisStore(const std::string& host, int port, const ClientConfig& default_config);
    ~RedisStore() override;

    bool consume(const std::string& client_id,
                 int tokens,
                 BucketStat& stat,
                 ClientConfig* applied_config) override;
    ClientConfig getClientConfig(const std::string& client_id) override;
    void updateQuota(const std::string& client_id,
                     const QuotaUpdate& update,
                     BucketStat& stat,
                     ClientConfig& applied_config) override;
    void upsertClientConfig(const std::string& client_id,
                            const ClientConfig& config,
                            bool reset_quota_state,
                            BucketStat& stat,
                            ClientConfig& applied_config) override;

private:
    redisContext* ctx_;
    std::mutex mtx_;
    ClientConfig default_config_;
    std::string consume_sha_;
    std::string update_sha_;
    std::string reconcile_sha_;

    void loadLuaScripts();
    ClientConfig loadClientConfigUnlocked(const std::string& client_id);
    static ClientConfig normalizeConfig(const ClientConfig& config, const ClientConfig& fallback);
    static std::string configKey(const std::string& client_id);
    static std::string bucketKey(const std::string& client_id);
};
