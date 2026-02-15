
#pragma once
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <memory>

struct Metrics {
    std::shared_ptr<prometheus::Registry> registry;
    
    // Store family pointers to keep them alive
    std::unique_ptr<prometheus::Family<prometheus::Counter>> requests_family;
    std::unique_ptr<prometheus::Family<prometheus::Counter>> denied_family;
    std::unique_ptr<prometheus::Family<prometheus::Histogram>> latency_family;
    
    prometheus::Counter* requests_total;
    prometheus::Counter* denied_total;
    prometheus::Histogram* latency;
    
    Metrics();
};
