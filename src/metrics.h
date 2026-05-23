
#pragma once
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <memory>

struct Metrics {
    std::shared_ptr<prometheus::Registry> registry;
    
    // Families are owned by the registry; these pointers provide stable access.
    prometheus::Family<prometheus::Counter>* requests_family;
    prometheus::Family<prometheus::Counter>* allowed_family;
    prometheus::Family<prometheus::Counter>* rejected_family;
    prometheus::Family<prometheus::Histogram>* latency_family;
    
    prometheus::Counter* requests_total;
    prometheus::Counter* allowed_total;
    prometheus::Counter* rejected_total;
    prometheus::Histogram* latency;
    
    Metrics();
};
