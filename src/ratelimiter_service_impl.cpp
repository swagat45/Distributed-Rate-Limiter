#include "ratelimiter_service_impl.h"
#include <chrono>
#include <exception>

namespace {

class ScopedLatency {
public:
    explicit ScopedLatency(Metrics* metrics)
        : metrics_(metrics), start_(std::chrono::steady_clock::now()) {
        metrics_->requests_total->Increment();
    }

    ~ScopedLatency() {
        const auto end = std::chrono::steady_clock::now();
        const double ms =
            std::chrono::duration<double, std::milli>(end - start_).count();
        metrics_->latency->Observe(ms);
    }

private:
    Metrics* metrics_;
    std::chrono::steady_clock::time_point start_;
};

bool isBlank(const std::string& value) {
    return value.empty();
}

}  // namespace

RateLimiterServiceImpl::RateLimiterServiceImpl(std::unique_ptr<ITokenStore> store,
                                               Metrics* metrics)
    : store_(std::move(store)), metrics_(metrics) {}

grpc::Status RateLimiterServiceImpl::CheckQuota(grpc::ServerContext* context,
                                                const ratelimiter::CheckQuotaRequest* request,
                                                ratelimiter::CheckQuotaReply* response) {
    (void)context;
    ScopedLatency scoped_latency(metrics_);

    if (isBlank(request->client_id())) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "client_id must not be empty");
    }
    if (request->hits() <= 0) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "hits must be positive");
    }

    try {
        BucketStat stat{};
        const bool allowed =
            store_->consume(request->client_id(), request->hits(), stat, nullptr);

        if (allowed) {
            metrics_->allowed_total->Increment();
        } else {
            metrics_->rejected_total->Increment();
        }

        response->set_allowed(allowed);
        response->set_remaining(stat.remaining);
        response->set_reset_after_ms(stat.reset_after_ms);
        return grpc::Status::OK;
    } catch (const std::invalid_argument& ex) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    } catch (const std::exception& ex) {
        return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
    }
}

grpc::Status RateLimiterServiceImpl::UpdateQuota(grpc::ServerContext* context,
                                                 const ratelimiter::UpdateQuotaRequest* request,
                                                 ratelimiter::UpdateQuotaReply* response) {
    (void)context;
    ScopedLatency scoped_latency(metrics_);

    if (isBlank(request->client_id())) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "client_id must not be empty");
    }

    QuotaUpdate update{};
    switch (request->quota_operation_case()) {
    case ratelimiter::UpdateQuotaRequest::kTokenDelta:
        update.mode = QuotaUpdate::Mode::kAddTokens;
        update.value = request->token_delta();
        break;
    case ratelimiter::UpdateQuotaRequest::kSetTokens:
        update.mode = QuotaUpdate::Mode::kSetTokens;
        update.value = request->set_tokens();
        break;
    case ratelimiter::UpdateQuotaRequest::QUOTA_OPERATION_NOT_SET:
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "quota operation must be set");
    }
    update.reset_refill_time = request->reset_refill_time();

    try {
        BucketStat stat{};
        ClientConfig applied_config{};
        store_->updateQuota(request->client_id(), update, stat, applied_config);

        response->set_applied(true);
        response->set_remaining(stat.remaining);
        response->set_reset_after_ms(stat.reset_after_ms);
        response->set_capacity(applied_config.capacity);
        response->set_refill_rate_per_sec(applied_config.refill_rate_per_sec);
        response->set_ttl_seconds(applied_config.ttl_seconds);
        return grpc::Status::OK;
    } catch (const std::invalid_argument& ex) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    } catch (const std::exception& ex) {
        return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
    }
}

grpc::Status RateLimiterServiceImpl::UpsertClientConfig(
    grpc::ServerContext* context,
    const ratelimiter::UpsertClientConfigRequest* request,
    ratelimiter::UpsertClientConfigReply* response) {
    (void)context;
    ScopedLatency scoped_latency(metrics_);

    if (isBlank(request->client_id())) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "client_id must not be empty");
    }

    ClientConfig requested_config{
        request->capacity(),
        request->refill_rate_per_sec(),
        request->ttl_seconds(),
    };

    try {
        BucketStat stat{};
        ClientConfig applied_config{};
        store_->upsertClientConfig(request->client_id(),
                                   requested_config,
                                   request->reset_quota_state(),
                                   stat,
                                   applied_config);

        response->set_applied(true);
        response->set_capacity(applied_config.capacity);
        response->set_refill_rate_per_sec(applied_config.refill_rate_per_sec);
        response->set_ttl_seconds(applied_config.ttl_seconds);
        response->set_remaining(stat.remaining);
        response->set_reset_after_ms(stat.reset_after_ms);
        return grpc::Status::OK;
    } catch (const std::invalid_argument& ex) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    } catch (const std::exception& ex) {
        return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
    }
}
