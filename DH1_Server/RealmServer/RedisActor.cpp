#include "pch.h"
#include "RedisActor.h"

bool RedisActor::Initialize(const std::string& connectionUri)
{
	try
	{
		const sw::redis::Uri uri(connectionUri);

		sw::redis::ConnectionOptions options = uri.connection_options();
		sw::redis::ConnectionPoolOptions poolOptions = uri.connection_pool_options();

		mRedis = std::make_unique<sw::redis::Redis>(options, poolOptions);

		const std::string pingReply = mRedis->ping();
		NET_ENGINE_LOG_INFO("[RedisActor] Initialized. Ping reply : {}", pingReply);
		return true;
	}
	catch (const sw::redis::Error& e)
	{
		NET_ENGINE_LOG_ERROR("[RedisActor] Initialization Failed : {}", e.what());
		return false;
	}
}

bool RedisActor::SetExpireString(const std::string& key, const std::string& value, const int64 ttlSeconds) const
{
	if (!mRedis)
	{
		return false;
	}

	try
	{
		const bool isSet = mRedis->set(key, value, std::chrono::seconds(ttlSeconds));
		return isSet;
	}
	catch (const sw::redis::Error& e)
	{
		NET_ENGINE_LOG_ERROR("[RedisActor] SetExpire Error : {}", e.what());
		return false;
	}
}
