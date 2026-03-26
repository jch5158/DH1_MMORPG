#include "pch.h"
#include "RedisActor.h"
#include "RedisService.h"
#include <CrashReporter.h>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RedisRealmRegistration, realmId, realmName, currentPlayers, maxPlayers, status)

RedisService::RedisService(const std::string& connectionUri, ActorServiceRef pActorService)
	: mRedisActorId(0)
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

void RedisService::UpdateRealmRegistration(const RedisRealmRegistration& registration, const int64 ttlSeconds)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argService = shared_from_this(), argInfo = registration, argTtl = ttlSeconds]()->void
		{
			const auto pRedis = std::static_pointer_cast<RedisActor>(argService->mpActorService->GetActorRef(argService->mRedisActorId));
			if (pRedis == nullptr)
			{
				NET_ENGINE_LOG_ERROR("RedisService::UpdateRealmRegistration - RedisActor is null");
				return;
			}

			const std::string redisKey = "Realm:" + std::to_string(argInfo.realmId);

			const nlohmann::json jsonInfo = argInfo;
			const std::string redisValue = jsonInfo.dump();

			const bool isSuccess = pRedis->SetExpireString(redisKey, redisValue, argTtl);
			if (isSuccess)
			{
				NET_ENGINE_LOG_TRACE("RedisService - Realm registered: {}, players: {}/{}", argInfo.realmName, argInfo.currentPlayers, argInfo.maxPlayers);
			}
			else
			{
				NET_ENGINE_LOG_ERROR("RedisService - Realm registration failed: {}", argInfo.realmName);
			}
		});
}
