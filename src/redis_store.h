
#pragma once
#include "backend_interface.h"
#include <hiredis/hiredis.h>
#include <mutex>

class RedisStore : public ITokenStore {
public:
    RedisStore(const std::string &host,int port,int capacity,int refill);
    ~RedisStore() override;
    bool consume(const std::string& key,int tokens,BucketStat &stat) override;
private:
    redisContext *ctx_;
    std::mutex mtx_;
    int capacity_;
    int refill_;
    std::string lua_sha_;
    void loadLuaScript();
};
