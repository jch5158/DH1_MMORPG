#include "pch.h"
#include "RedisActor.h"
#include "RedisService.h"
#include <CrashReporter.h>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RedisGatewayInfo, gatewayId, ip, port, status, currentSessionCount)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RedisRealmInfo, realmId, realmName, currentPlayers, maxPlayers, status)

RedisService::RedisService(const std::string& connectionUri, ActorServiceRef pActorService)
	:mRedisActorId(0)
	, mpActorService(std::move(pActorService))
{
	const auto pRedis = cpp_net_engine::MakeShared<RedisActor>();
	pRedis->Initialize(connectionUri);
	if (mpActorService->RegisterActor(std::static_pointer_cast<Actor>(pRedis)) == false)
	{
		NET_ENGINE_LOG_FATAL("RedisService::RedisService - RegisterActor failed");
		CrashReporter::Crash();
	}

	mRedisActorId = pRedis->GetId();
}

void RedisService::UpdateGatewayInfo(const RedisGatewayInfo& gatewayInfo)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argRedisService = shared_from_this(), argInfo = gatewayInfo]()->void
		{
			const RedisActorRef pRedis = argRedisService->GetRedisActorRef();
			const std::string redisKey = "Gateway:" + std::to_string(argInfo.gatewayId);

			const nlohmann::json jsonInfo = argInfo;
			const std::string redisValue = jsonInfo.dump();

			(void)pRedis->SetExpireString(redisKey, redisValue, 10);
		});
}

RedisActorRef RedisService::GetRedisActorRef()
{
	RedisActorRef pRedis = std::static_pointer_cast<RedisActor>(mpActorService->GetActorRef(mRedisActorId));
	return pRedis;
}

void RedisService::SetStringAsync(std::string key, std::string value, SetStringCallback callback)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argKey = std::move(key), argValue = std::move(value), argCallback = std::move(callback), argService = shared_from_this()]()->void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			if (pRedis == nullptr)
			{
				argCallback(false);
				return;
			}

			const bool isSuccess = pRedis->SetString(argKey, argValue);
			argCallback(isSuccess);
		});
}

void RedisService::SetExpireStringAsync(std::string key, std::string value, const int64 ttlSeconds, SetStringCallback callback)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argKey = std::move(key), argValue = std::move(value), argCallback = std::move(callback), argTtlSeconds = ttlSeconds, argService = shared_from_this()]()->void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			if (pRedis == nullptr)
			{
				argCallback(false);
				return;
			}

			const bool isSuccess = pRedis->SetExpireString(argKey, argValue, argTtlSeconds);
			argCallback(isSuccess);
		});
}

void RedisService::GetStringAsync(std::string key, GetStringCallback callback)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argKey = std::move(key), argCallback = std::move(callback), argService = shared_from_this()]()->void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			if (pRedis == nullptr)
			{
				argCallback(std::nullopt);
				return;
			}

			const auto retValue = pRedis->GetString(argKey);
			argCallback(retValue);
		});
}

void RedisService::GetDelStringAsync(std::string key, GetStringCallback callback)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argKey = std::move(key), argCallback = std::move(callback), argService = shared_from_this()]()->void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			if (pRedis == nullptr)
			{
				argCallback(std::nullopt);
				return;
			}

			const auto retValue = pRedis->GetDelString(argKey);
			argCallback(retValue);
		});
}

void RedisService::DeleteKeyAsync(std::string key, DeleteKeyCallback callback)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argKey = std::move(key), argCallback = std::move(callback), argService = shared_from_this()]()->void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			if (pRedis == nullptr)
			{
				argCallback(false);
				return;
			}

			const bool isSuccess = pRedis->DeleteKey(argKey);
			argCallback(isSuccess);
		});
}

void RedisService::GetRealmListAsync(GetRealmListCallback callback)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argCallback = std::move(callback), argService = shared_from_this()]()->void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			if (pRedis == nullptr)
			{
				argCallback({});
				return;
			}

			Vector<RedisRealmInfo> realms;
			const auto keys = pRedis->Keys("Realm:*");

			for (const auto& key : keys)
			{
				const auto value = pRedis->GetString(key);
				if (!value.has_value())
				{
					continue;
				}

				try
				{
					const auto json = nlohmann::json::parse(value.value());
					RedisRealmInfo info = json.get<RedisRealmInfo>();
					realms.emplace_back(std::move(info));
				}
				catch (const nlohmann::json::exception& e)
				{
					NET_ENGINE_LOG_ERROR("RedisService::GetRealmListAsync - JSON parse error: {}", e.what());
				}
			}

			argCallback(realms);
		});
}

void RedisService::GetRealmInfoAsync(const int32 realmId, GetRealmInfoCallback callback)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argRealmId = realmId, argCallback = std::move(callback), argService = shared_from_this()]()->void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			if (pRedis == nullptr)
			{
				argCallback(false, {});
				return;
			}

			const std::string key = "Realm:" + std::to_string(argRealmId);
			const auto value = pRedis->GetString(key);
			if (!value.has_value())
			{
				argCallback(false, {});
				return;
			}

			try
			{
				const auto json = nlohmann::json::parse(value.value());
				RedisRealmInfo info = json.get<RedisRealmInfo>();
				argCallback(true, info);
			}
			catch (const nlohmann::json::exception& e)
			{
				NET_ENGINE_LOG_ERROR("RedisService::GetRealmInfoAsync - JSON parse error: {}", e.what());
				argCallback(false, {});
			}
		});
}
