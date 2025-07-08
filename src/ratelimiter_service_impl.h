
#pragma once
#include "proto/ratelimit.grpc.pb.h"
#include "backend_interface.h"
#include "metrics.h"
#include <memory>

class RateLimiterServiceImpl final : public ratelimiter::RateLimiter::Service {
public:
    RateLimiterServiceImpl(std::unique_ptr<ITokenStore> store, Metrics* metrics);

    grpc::Status CheckQuota(grpc::ServerContext* context,
                            const ratelimiter::RateLimitRequest* request,
                            ratelimiter::RateLimitReply* response) override;
private:
    std::unique_ptr<ITokenStore> store_;
    Metrics* metrics_;
};
