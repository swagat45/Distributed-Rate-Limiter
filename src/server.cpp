
#include <grpcpp/grpcpp.h>
#include "ratelimiter_service_impl.h"
#include "redis_store.h"
#include "metrics.h"
#include <prometheus/exposer.h>
#include <thread>

int main(int argc,char** argv){
    int capacity=10, refill=5;
    std::string srv_addr("0.0.0.0:50051"), redis_host("redis"), prom_addr("0.0.0.0:9102");
    if(const char* env=getenv("BUCKET_CAP")) capacity=std::stoi(env);
    if(const char* env=getenv("BUCKET_REFILL")) refill=std::stoi(env);
    if(const char* env=getenv("BIND_ADDR")) srv_addr=env;
    if(const char* env=getenv("REDIS_HOST")) redis_host=env;
    Metrics metrics;

    // expose prometheus
    prometheus::Exposer exposer{prom_addr};
    exposer.RegisterCollectable(metrics.registry);

    auto store=std::make_unique<RedisStore>(redis_host,6379,capacity,refill);
    RateLimiterServiceImpl service(std::move(store),&metrics);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(srv_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout<<"Server at "<<srv_addr<<" metrics at "<<prom_addr<<std::endl;
    server->Wait();
    return 0;
}
