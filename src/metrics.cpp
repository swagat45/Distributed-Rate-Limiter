
#include "metrics.h"

Metrics::Metrics(){
    registry = std::make_shared<prometheus::Registry>();
    
    requests_family = std::make_unique<prometheus::Family<prometheus::Counter>>(
        prometheus::BuildCounter()
            .Name("ratelimiter_requests_total")
            .Help("Total requests")
            .Register(*registry));
    
    denied_family = std::make_unique<prometheus::Family<prometheus::Counter>>(
        prometheus::BuildCounter()
            .Name("ratelimiter_denied_total")
            .Help("Denied requests")
            .Register(*registry));
    
    latency_family = std::make_unique<prometheus::Family<prometheus::Histogram>>(
        prometheus::BuildHistogram()
            .Name("ratelimiter_latency_ms")
            .Help("Latency in ms")
            .Register(*registry));
    
    requests_total = &requests_family->Add({});
    denied_total = &denied_family->Add({});
    latency = &latency_family->Add({}, prometheus::Histogram::BucketBoundaries{1,2,5,10,20,50,100});
}
