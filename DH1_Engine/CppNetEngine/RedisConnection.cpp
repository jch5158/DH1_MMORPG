#include "pch.h"
#include "RedisConnection.h"

RedisConnection::RedisConnection()
	: mRedis()
{
}

bool RedisConnection::Initialize(const std::string& connectionUri)
{
	try
	{
		const sw::redis::Uri uri(connectionUri);
		sw::redis::ConnectionOptions options = uri.connection_options();
		sw::redis::ConnectionPoolOptions poolOptions = uri.connection_pool_options();

		mRedis = std::make_unique<sw::redis::Redis>(options, poolOptions);

		const std::string pingReply = mRedis->ping();
		NET_ENGINE_LOG_INFO("[RedisConnection] Initialized. Ping reply : {}", pingReply);
		return true;
	}
	catch (const sw::redis::Error& e)
	{
		NET_ENGINE_LOG_ERROR("[RedisConnection] Initialization Failed : {}", e.what());
		return false;
	}
}

int64 RedisConnection::IncrBy(const std::string& key, int64 amount) const
{
	if (!mRedis)
	{
		return 0;
	}

	try
	{
		const int64 result = mRedis->incrby(key, amount);
		return result;
	}
	catch (const sw::redis::Error& e)
	{
		NET_ENGINE_LOG_ERROR("[RedisConnection] IncrBy Failed : {}", e.what());
		return 0;
	}
}
