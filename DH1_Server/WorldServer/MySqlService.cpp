#include "pch.h"
#include "MySqlService.h"

MySqlService::MySqlService(const MySqlConfig& config, ActorServiceRef pActorService)
	: mbInitialized(false)
	, mpActorService(std::move(pActorService))
{
	const int32 poolSize = std::max(1, config.poolSize);
	Vector<uint64> actorIds;

	for (int32 i = 0; i < poolSize; ++i)
	{
		const auto pMySql = cpp_net_engine::MakeShared<MySqlActor>();
		if (pMySql->Initialize(config) == false)
		{
			NET_ENGINE_LOG_ERROR("MySqlService - MySqlActor[{}] initialization failed, MySQL disabled", i);
			return;
		}

		if (mpActorService->RegisterActor(std::static_pointer_cast<Actor>(pMySql)) == false)
		{
			NET_ENGINE_LOG_ERROR("MySqlService - RegisterActor[{}] failed, MySQL disabled", i);
			return;
		}

		actorIds.push_back(pMySql->GetId());
	}

	mpLoadBalancer = std::make_unique<ActorLoadBalancer>(actorIds);
	mbInitialized = true;
	NET_ENGINE_LOG_INFO("MySqlService - Initialized with {} connection(s), database: {}", poolSize, config.database);
}

void MySqlService::ReleaseRoutingKey(const uint64 routingKey)
{
	if (mpLoadBalancer != nullptr)
	{
		mpLoadBalancer->Release(routingKey);
	}
}

MySqlActorRef MySqlService::GetMySqlActorRef(const uint64 actorId)
{
	return std::static_pointer_cast<MySqlActor>(mpActorService->GetActorRef(actorId));
}
