
#include "ratelimiter_service_impl.h"
#include <chrono>

RateLimiterServiceImpl::RateLimiterServiceImpl(std::unique_ptr<ITokenStore> store, Metrics* metrics)
    :store_(std::move(store)),metrics_(metrics){}

grpc::Status RateLimiterServiceImpl::CheckQuota(grpc::ServerContext* context,
                                               const ratelimiter::RateLimitRequest* request,
                                               ratelimiter::RateLimitReply* response){
    auto start=std::chrono::steady_clock::now();
    metrics_->requests_total->Increment();

    BucketStat stat;
    bool allowed=store_->consume(request->key(),request->hits(),stat);

    if(!allowed) metrics_->denied_total->Increment();

    response->set_allowed(allowed);
    response->set_remaining(stat.remaining);
    response->set_reset_after_ms(stat.reset_after_ms);

    auto end=std::chrono::steady_clock::now();
    double ms=std::chrono::duration<double,std::milli>(end-start).count();
    metrics_->latency->Observe(ms);

    return grpc::Status::OK;
}
