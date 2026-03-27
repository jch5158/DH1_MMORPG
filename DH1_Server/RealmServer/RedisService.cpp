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

void RedisService::UpdateRealmRegistration(const RedisRealmRegistration& info, const int64 ttlSeconds)
{
	mpActorService->GetActorDispatcher().Post(mRedisActorId, [argService = shared_from_this(), argInfo = info, argTtl = ttlSeconds]() -> void
		{
			const RedisActorRef pRedis = argService->GetRedisActorRef();
			const std::string redisKey = "Realm:" + std::to_string(argInfo.realmId);

			const nlohmann::json jsonInfo = argInfo;
			const std::string redisValue = jsonInfo.dump();

			(void)pRedis->SetExpireString(redisKey, redisValue, argTtl);
		});
}

RedisActorRef RedisService::GetRedisActorRef()
{
	return std::static_pointer_cast<RedisActor>(mpActorService->GetActorRef(mRedisActorId));
}
