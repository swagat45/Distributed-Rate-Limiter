#include "src/metrics.h"
#include "src/ratelimiter_service_impl.h"
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

namespace {

class FakeTokenStore final : public ITokenStore {
public:
    bool allow_next = true;
    bool throw_on_consume = false;
    bool throw_on_update = false;
    bool throw_on_upsert = false;
    BucketStat next_stat{4, 750};
    ClientConfig next_config{10, 5, 60};
    QuotaUpdate last_update{QuotaUpdate::Mode::kAddTokens, 0, false};
    std::string last_client_id;
    int consume_calls = 0;
    int update_calls = 0;
    int upsert_calls = 0;

    bool consume(const std::string& client_id,
                 int tokens,
                 BucketStat& stat,
                 ClientConfig* applied_config) override {
        (void)tokens;
        ++consume_calls;
        last_client_id = client_id;
        if (throw_on_consume) {
            throw std::runtime_error("consume failed");
        }
        stat = next_stat;
        if (applied_config != nullptr) {
            *applied_config = next_config;
        }
        return allow_next;
    }

    ClientConfig getClientConfig(const std::string& client_id) override {
        last_client_id = client_id;
        return next_config;
    }

    void updateQuota(const std::string& client_id,
                     const QuotaUpdate& update,
                     BucketStat& stat,
                     ClientConfig& applied_config) override {
        ++update_calls;
        last_client_id = client_id;
        last_update = update;
        if (throw_on_update) {
            throw std::runtime_error("update failed");
        }
        stat = next_stat;
        applied_config = next_config;
    }

    void upsertClientConfig(const std::string& client_id,
                            const ClientConfig& config,
                            bool reset_quota_state,
                            BucketStat& stat,
                            ClientConfig& applied_config) override {
        ++upsert_calls;
        last_client_id = client_id;
        next_config = config;
        if (throw_on_upsert) {
            throw std::runtime_error("upsert failed");
        }
        if (reset_quota_state) {
            stat = BucketStat{config.capacity, 0};
        } else {
            stat = next_stat;
        }
        applied_config = config;
    }
};

class RateLimiterServiceFixture : public ::testing::Test {
protected:
    void SetUp() override {
        raw_store_ = new FakeTokenStore();
        service_ = std::make_unique<RateLimiterServiceImpl>(
            std::unique_ptr<ITokenStore>(raw_store_), &metrics_);
    }

    FakeTokenStore* raw_store_ = nullptr;
    Metrics metrics_;
    std::unique_ptr<RateLimiterServiceImpl> service_;
};

}  // namespace

TEST_F(RateLimiterServiceFixture, CheckQuotaAllowedIncrementsAllowedMetric) {
    grpc::ServerContext context;
    ratelimiter::CheckQuotaRequest request;
    ratelimiter::CheckQuotaReply reply;
    request.set_client_id("client-a");
    request.set_hits(1);
    raw_store_->allow_next = true;
    const double allowed_before = metrics_.allowed_total->Value();

    const grpc::Status status = service_->CheckQuota(&context, &request, &reply);

    EXPECT_TRUE(status.ok());
    EXPECT_TRUE(reply.allowed());
    EXPECT_EQ(allowed_before + 1.0, metrics_.allowed_total->Value());
}

TEST_F(RateLimiterServiceFixture, CheckQuotaRejectedIncrementsRejectedMetric) {
    grpc::ServerContext context;
    ratelimiter::CheckQuotaRequest request;
    ratelimiter::CheckQuotaReply reply;
    request.set_client_id("client-a");
    request.set_hits(2);
    raw_store_->allow_next = false;
    raw_store_->next_stat = BucketStat{0, 400};
    const double rejected_before = metrics_.rejected_total->Value();

    const grpc::Status status = service_->CheckQuota(&context, &request, &reply);

    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(reply.allowed());
    EXPECT_EQ(0, reply.remaining());
    EXPECT_EQ(rejected_before + 1.0, metrics_.rejected_total->Value());
}

TEST_F(RateLimiterServiceFixture, CheckQuotaRejectsMalformedRequest) {
    grpc::ServerContext context;
    ratelimiter::CheckQuotaRequest request;
    ratelimiter::CheckQuotaReply reply;
    request.set_client_id("");
    request.set_hits(0);

    const grpc::Status status = service_->CheckQuota(&context, &request, &reply);

    EXPECT_EQ(grpc::StatusCode::INVALID_ARGUMENT, status.error_code());
    EXPECT_EQ(0, raw_store_->consume_calls);
}

TEST_F(RateLimiterServiceFixture, UpdateQuotaRequiresOperation) {
    grpc::ServerContext context;
    ratelimiter::UpdateQuotaRequest request;
    ratelimiter::UpdateQuotaReply reply;
    request.set_client_id("client-a");

    const grpc::Status status = service_->UpdateQuota(&context, &request, &reply);

    EXPECT_EQ(grpc::StatusCode::INVALID_ARGUMENT, status.error_code());
    EXPECT_EQ(0, raw_store_->update_calls);
}

TEST_F(RateLimiterServiceFixture, UpdateQuotaMapsSetTokensOperation) {
    grpc::ServerContext context;
    ratelimiter::UpdateQuotaRequest request;
    ratelimiter::UpdateQuotaReply reply;
    request.set_client_id("client-a");
    request.set_set_tokens(6);
    request.set_reset_refill_time(true);
    raw_store_->next_stat = BucketStat{6, 800};
    raw_store_->next_config = ClientConfig{8, 4, 120};

    const grpc::Status status = service_->UpdateQuota(&context, &request, &reply);

    EXPECT_TRUE(status.ok());
    EXPECT_EQ(QuotaUpdate::Mode::kSetTokens, raw_store_->last_update.mode);
    EXPECT_EQ(6, raw_store_->last_update.value);
    EXPECT_TRUE(raw_store_->last_update.reset_refill_time);
    EXPECT_EQ(6, reply.remaining());
    EXPECT_EQ(8, reply.capacity());
    EXPECT_EQ(4, reply.refill_rate_per_sec());
}

TEST_F(RateLimiterServiceFixture, UpsertClientConfigRejectsEmptyClientId) {
    grpc::ServerContext context;
    ratelimiter::UpsertClientConfigRequest request;
    ratelimiter::UpsertClientConfigReply reply;
    request.set_client_id("");
    request.set_capacity(5);
    request.set_refill_rate_per_sec(2);
    request.set_ttl_seconds(60);

    const grpc::Status status = service_->UpsertClientConfig(&context, &request, &reply);

    EXPECT_EQ(grpc::StatusCode::INVALID_ARGUMENT, status.error_code());
    EXPECT_EQ(0, raw_store_->upsert_calls);
}

TEST_F(RateLimiterServiceFixture, UpsertClientConfigReturnsAppliedState) {
    grpc::ServerContext context;
    ratelimiter::UpsertClientConfigRequest request;
    ratelimiter::UpsertClientConfigReply reply;
    request.set_client_id("client-b");
    request.set_capacity(12);
    request.set_refill_rate_per_sec(3);
    request.set_ttl_seconds(90);
    request.set_reset_quota_state(true);

    const grpc::Status status = service_->UpsertClientConfig(&context, &request, &reply);

    EXPECT_TRUE(status.ok());
    EXPECT_TRUE(reply.applied());
    EXPECT_EQ(12, reply.capacity());
    EXPECT_EQ(3, reply.refill_rate_per_sec());
    EXPECT_EQ(90, reply.ttl_seconds());
    EXPECT_EQ(12, reply.remaining());
}

TEST_F(RateLimiterServiceFixture, StoreFailuresBecomeInternalErrors) {
    grpc::ServerContext context;
    ratelimiter::CheckQuotaRequest request;
    ratelimiter::CheckQuotaReply reply;
    request.set_client_id("client-a");
    request.set_hits(1);
    raw_store_->throw_on_consume = true;

    const grpc::Status status = service_->CheckQuota(&context, &request, &reply);

    EXPECT_EQ(grpc::StatusCode::INTERNAL, status.error_code());
}
