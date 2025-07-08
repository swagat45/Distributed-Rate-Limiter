
#pragma once
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

struct Metrics {
    std::shared_ptr<prometheus::Registry> registry;
    prometheus::Counter* requests_total;
    prometheus::Counter* denied_total;
    prometheus::Histogram* latency;
    Metrics();
};
