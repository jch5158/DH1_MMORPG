#include "pch.h"
#include "RedisActor.h"

bool RedisActor::Initialize(const std::string& connectionUri)
{
    try
    {
        const sw::redis::Uri uri(connectionUri);

        sw::redis::ConnectionOptions options = uri.connection_options();
        sw::redis::ConnectionPoolOptions poolOptions = uri.connection_pool_options();

        // Uri 로 생략 예정
        //poolOptions.size = 10;

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

bool RedisActor::SetString(const std::string& key, const std::string& value) const
{
    if (!mRedis)
    {
	    return false;
    }

    try
    {
        const bool isSet = mRedis->set(key, value);
        return isSet;
    }
    catch (const sw::redis::Error& e)
    {
        NET_ENGINE_LOG_ERROR("[RedisActor] Set Error : {}", e.what());
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
        NET_ENGINE_LOG_ERROR("[RedisActor] Set Error : {}", e.what());
        return false;
    }
}

std::optional<std::string> RedisActor::GetString(const std::string& key) const
{
    if (!mRedis)
    {
	    return std::nullopt;
    }

    try
    {
        if (auto val = mRedis->get(key))
        {
            return *val;
        }
        
    	return std::nullopt;
    }
    catch (const sw::redis::Error& e)
    {
        NET_ENGINE_LOG_ERROR("[RedisActor] Get Error : {}", e.what());
        return std::nullopt;
    }
}

std::optional<std::string> RedisActor::GetDelString(const std::string& key) const
{
    if (!mRedis)
    {
        return std::nullopt;
    }

    try
    {
        if (auto val = mRedis->command<sw::redis::OptionalString>("GETDEL", key))
        {
            return *val;
        }

        return std::nullopt;
    }
    catch (const sw::redis::Error& e)
    {
        NET_ENGINE_LOG_ERROR("[RedisActor] GetDel Error : {}", e.what());
        return std::nullopt;
    }
}

bool RedisActor::DeleteKey(const std::string& key) const
{
    if (!mRedis)
    {
        return false;
    }

    try
    {
        const int64 delCount = mRedis->del(key);
        return delCount > 0;
    }
    catch (const sw::redis::Error& e)
    {
        NET_ENGINE_LOG_ERROR("[RedisActor] Delete Error : {}", e.what());
        return false;
    }
}
