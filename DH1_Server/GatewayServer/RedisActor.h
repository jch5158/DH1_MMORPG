#pragma once
#include <Actor.h>
#include <sw/redis++/redis++.h>

class RedisActor final : public Actor
{
public:
    RedisActor() = default;
    virtual ~RedisActor() override = default;

    RedisActor(const RedisActor&) = delete;
    RedisActor& operator=(const RedisActor&) = delete;
    RedisActor(RedisActor&&) = delete;
    RedisActor& operator=(RedisActor&&) = delete;

    bool Initialize(const std::string& connectionUri);

    bool SetString(const std::string& key, const std::string& value, const int64 ttlSeconds) const;
    std::optional<std::string> GetString(const std::string& key) const;

private:

    std::unique_ptr<sw::redis::Redis> mRedis;
};