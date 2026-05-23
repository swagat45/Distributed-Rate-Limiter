#include <grpcpp/grpcpp.h>
#include "proto/ratelimit.grpc.pb.h"
#include <iostream>

int main(int argc, char** argv) {
    const std::string addr(argc > 1 ? argv[1] : "localhost:50051");
    auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    auto stub = ratelimiter::RateLimiter::NewStub(channel);

    const std::string client_id = argc > 2 ? argv[2] : "demo-client";

    ratelimiter::UpsertClientConfigRequest config_request;
    config_request.set_client_id(client_id);
    config_request.set_capacity(5);
    config_request.set_refill_rate_per_sec(2);
    config_request.set_ttl_seconds(300);
    config_request.set_reset_quota_state(true);

    ratelimiter::UpsertClientConfigReply config_reply;
    grpc::ClientContext config_context;
    grpc::Status config_status =
        stub->UpsertClientConfig(&config_context, config_request, &config_reply);
    if (!config_status.ok()) {
        std::cerr << "UpsertClientConfig failed: "
                  << config_status.error_message() << std::endl;
        return 1;
    }

    std::cout << "Configured client '" << client_id << "'"
              << " capacity=" << config_reply.capacity()
              << " refill_rate_per_sec=" << config_reply.refill_rate_per_sec()
              << " ttl_seconds=" << config_reply.ttl_seconds()
              << " remaining=" << config_reply.remaining()
              << std::endl;

    ratelimiter::CheckQuotaRequest quota_request;
    quota_request.set_client_id(client_id);
    quota_request.set_hits(1);

    for (int i = 0; i < 7; ++i) {
        ratelimiter::CheckQuotaReply quota_reply;
        grpc::ClientContext quota_context;
        grpc::Status quota_status =
            stub->CheckQuota(&quota_context, quota_request, &quota_reply);
        if (!quota_status.ok()) {
            std::cerr << "CheckQuota failed: " << quota_status.error_message() << std::endl;
            return 1;
        }

        std::cout << "Request " << (i + 1)
                  << " allowed=" << quota_reply.allowed()
                  << " remaining=" << quota_reply.remaining()
                  << " reset_after_ms=" << quota_reply.reset_after_ms()
                  << std::endl;
    }

    ratelimiter::UpdateQuotaRequest update_request;
    update_request.set_client_id(client_id);
    update_request.set_token_delta(3);
    update_request.set_reset_refill_time(true);

    ratelimiter::UpdateQuotaReply update_reply;
    grpc::ClientContext update_context;
    grpc::Status update_status =
        stub->UpdateQuota(&update_context, update_request, &update_reply);
    if (!update_status.ok()) {
        std::cerr << "UpdateQuota failed: " << update_status.error_message() << std::endl;
        return 1;
    }

    std::cout << "Quota updated"
              << " remaining=" << update_reply.remaining()
              << " capacity=" << update_reply.capacity()
              << " refill_rate_per_sec=" << update_reply.refill_rate_per_sec()
              << " reset_after_ms=" << update_reply.reset_after_ms()
              << std::endl;

    ratelimiter::CheckQuotaReply final_reply;
    grpc::ClientContext final_context;
    grpc::Status final_status =
        stub->CheckQuota(&final_context, quota_request, &final_reply);
    if (!final_status.ok()) {
        std::cerr << "Final CheckQuota failed: " << final_status.error_message()
                  << std::endl;
        return 1;
    }

    std::cout << "Post-update request"
              << " allowed=" << final_reply.allowed()
              << " remaining=" << final_reply.remaining()
              << " reset_after_ms=" << final_reply.reset_after_ms()
              << std::endl;
    return 0;
}
