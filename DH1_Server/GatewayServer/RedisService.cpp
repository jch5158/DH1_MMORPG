#include "pch.h"
#include "RedisActor.h"
#include "RedisService.h"
#include <CrashReporter.h>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RedisGatewayInfo, gatewayId, ip, port, status, currentSessionCount)

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
