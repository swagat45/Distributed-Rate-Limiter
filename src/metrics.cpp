
#include "metrics.h"
#include <prometheus/counter.h>
#include <prometheus/histogram.h>

Metrics::Metrics(){
    registry = std::make_shared<prometheus::Registry>();
    auto& counter_family = prometheus::BuildCounter()
        .Name("ratelimiter_requests_total")
        .Help("Total requests")
        .Register(*registry);
    requests_total=&counter_family.Add({});
    auto& denied_family = prometheus::BuildCounter()
        .Name("ratelimiter_denied_total")
        .Help("Denied requests")
        .Register(*registry);
    denied_total=&denied_family.Add({});
    auto& hist_family=prometheus::BuildHistogram()
        .Name("ratelimiter_latency_ms")
        .Help("Latency in ms")
        .Register(*registry);
    latency=&hist_family.Add({}, prometheus::Histogram::BucketBoundaries{1,2,5,10,20,50,100});
}
