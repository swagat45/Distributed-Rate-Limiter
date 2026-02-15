
#include <grpcpp/grpcpp.h>
#include "proto/ratelimit.grpc.pb.h"
#include <iostream>

int main(int argc,char** argv){
    std::string addr(argc>1?argv[1]:"localhost:50051");
    auto channel=grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    auto stub=ratelimiter::RateLimiter::NewStub(channel);

    ratelimiter::RateLimitRequest req;
    req.set_key("test");
    req.set_hits(1);

    for(int i=0;i<5;++i){
        ratelimiter::RateLimitReply resp;
        grpc::ClientContext ctx;
        auto status=stub->CheckQuota(&ctx,req,&resp);
        if(status.ok()){
            std::cout << "Request " << (i+1) << ": allowed=" << resp.allowed() 
                      << " remaining=" << resp.remaining() 
                      << " reset_after_ms=" << resp.reset_after_ms() << std::endl;
        }else{
            std::cerr << "Error: " << status.error_message() << std::endl;
        }
    }
    return 0;
}
