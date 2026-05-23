
#include <grpcpp/grpcpp.h>
#include "ratelimiter_service_impl.h"
#include "redis_store.h"
#include "metrics.h"
#include <prometheus/exposer.h>
#include <cstdlib>
#include <iostream>

int main(int argc,char** argv){
    (void)argc;
    (void)argv;
    int capacity=10, refill=5, ttl_seconds=3600;
    std::string srv_addr("0.0.0.0:50051"), redis_host("redis"), prom_addr("0.0.0.0:9102");
    if(const char* env=getenv("BUCKET_CAP")) capacity=std::stoi(env);
    if(const char* env=getenv("BUCKET_REFILL")) refill=std::stoi(env);
    if(const char* env=getenv("BUCKET_TTL_SECONDS")) ttl_seconds=std::stoi(env);
    if(const char* env=getenv("BIND_ADDR")) srv_addr=env;
    if(const char* env=getenv("REDIS_HOST")) redis_host=env;
    if(const char* env=getenv("PROM_ADDR")) prom_addr=env;
    Metrics metrics;

    // expose prometheus
    prometheus::Exposer exposer{prom_addr};
    exposer.RegisterCollectable(metrics.registry);

    ClientConfig default_config{capacity, refill, ttl_seconds};
    auto store=std::make_unique<RedisStore>(redis_host,6379,default_config);
    RateLimiterServiceImpl service(std::move(store),&metrics);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(srv_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout<<"Server at "<<srv_addr<<" metrics at "<<prom_addr<<std::endl;
    server->Wait();
    return 0;
}
