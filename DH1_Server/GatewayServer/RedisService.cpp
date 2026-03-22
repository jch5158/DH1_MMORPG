#include "pch.h"
#include "RedisActor.h"
#include "RedisService.h"
#include <CrashReporter.h>
#include "nlohmann/json.hpp"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RedisGatewayInfo, gatewayId, ip, port, status, currentSessionCount)

RedisService::RedisService(const std::string& connectionUri, ActorSchedulerRef pScheduler)
	:mRedisActorId(0)
	, mActorManager()
	, mpRedisScheduler(std::move(pScheduler))
	, mActorDispatcher(*mpRedisScheduler, mActorManager)
{
	const auto pRedis = cpp_net_engine::MakeShared<RedisActor>();
	pRedis->Initialize(connectionUri);
	pRedis->Activate(mpRedisScheduler);
	if (mActorManager.AddActorRef(std::static_pointer_cast<Actor>(pRedis)) == false)
	{
		NET_ENGINE_LOG_FATAL("RedisService::RedisService - mActorManager.AddActorRef if failed");
		CrashReporter::Crash();
	}

	mRedisActorId = pRedis->GetId();
}

void RedisService::UpdateGatewayInfo(const RedisGatewayInfo& gatewayInfo)
{
	mActorDispatcher.Post(mRedisActorId, [argRedisService = shared_from_this(), argInfo = gatewayInfo]()->void
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
	RedisActorRef pRedis = std::static_pointer_cast<RedisActor>(mActorManager.GetActorRef(mRedisActorId));
	return pRedis;
}

void RedisService::SetStringAsync(std::string key, std::string value, SetStringCallback callback)
{
	mActorDispatcher.Post(mRedisActorId, [argKey = std::move(key), argValue = std::move(value), argCallback = std::move(callback), argService = shared_from_this()]()->void
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
	mActorDispatcher.Post(mRedisActorId, [argKey = std::move(key), argValue = std::move(value), argCallback = std::move(callback), argTtlSeconds = ttlSeconds, argService = shared_from_this()]()->void
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
	mActorDispatcher.Post(mRedisActorId, [argKey = std::move(key), argCallback = std::move(callback), argService = shared_from_this()]()->void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			if (pRedis == nullptr)
			{
				argCallback(std::nullopt); // 액터가 죽었으면 실패 처리
				return;
			}

			const auto retValue = pRedis->GetString(argKey);
			argCallback(retValue);
		});
}

void RedisService::DeleteKeyAsync(std::string key, DeleteKeyCallback callback)
{
	mActorDispatcher.Post(mRedisActorId, [argKey = std::move(key), argCallback = std::move(callback), argService = shared_from_this()]()->void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			if (pRedis == nullptr)
			{
				argCallback(false); // 액터가 죽었으면 실패 처리
				return;
			}

			const bool isSuccess = pRedis->DeleteKey(argKey);
			argCallback(isSuccess);
		});
}
