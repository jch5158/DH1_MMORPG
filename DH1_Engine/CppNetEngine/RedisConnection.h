#pragma once
#include <sw/redis++/redis++.h>

class RedisConnection
{
public:
	RedisConnection(const RedisConnection&) = delete;
	RedisConnection& operator=(const RedisConnection&) = delete;
	RedisConnection(RedisConnection&&) = delete;
	RedisConnection& operator=(RedisConnection&&) = delete;

	explicit RedisConnection();
	~RedisConnection() = default;

	bool Initialize(const std::string& connectionUri);
	int64 IncrBy(const std::string& key, int64 amount) const;

private:
	std::unique_ptr<sw::redis::Redis> mRedis;
};
