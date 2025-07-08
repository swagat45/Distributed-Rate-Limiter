
#pragma once
#include <string>
#include <memory>

struct BucketStat {
    int remaining;
    long long reset_after_ms;
};

class ITokenStore {
public:
    virtual ~ITokenStore() = default;
    virtual bool consume(const std::string& key,int tokens,BucketStat &stat)=0;
};
