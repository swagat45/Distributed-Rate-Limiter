
#include "metrics.h"

Metrics::Metrics(){
    registry = std::make_shared<prometheus::Registry>();
    
    requests_family = &prometheus::BuildCounter()
                           .Name("ratelimiter_requests_total")
                           .Help("Total requests")
                           .Register(*registry);
    
    allowed_family = &prometheus::BuildCounter()
                          .Name("ratelimiter_allowed_requests_total")
                          .Help("Allowed quota check requests")
                          .Register(*registry);

    rejected_family = &prometheus::BuildCounter()
                           .Name("ratelimiter_rejected_requests_total")
                           .Help("Rejected quota check requests")
                           .Register(*registry);
    
    latency_family = &prometheus::BuildHistogram()
                          .Name("ratelimiter_latency_ms")
                          .Help("Latency in ms")
                          .Register(*registry);
    
    requests_total = &requests_family->Add({});
    allowed_total = &allowed_family->Add({});
    rejected_total = &rejected_family->Add({});
    latency = &latency_family->Add(
        {}, prometheus::Histogram::BucketBoundaries{1,2,5,10,20,50,100});
}
