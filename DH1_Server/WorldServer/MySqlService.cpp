#include "pch.h"
#include "MySqlService.h"

MySqlService::MySqlService(const MySqlConfig& config, ActorServiceRef pActorService)
	: mbInitialized(false)
	, mMySqlActorId(0)
	, mpActorService(std::move(pActorService))
{
	const auto pMySql = cpp_net_engine::MakeShared<MySqlActor>();
	if (pMySql->Initialize(config) == false)
	{
		NET_ENGINE_LOG_ERROR("MySqlService - MySqlActor initialization failed, MySQL disabled");
		return;
	}

	if (mpActorService->RegisterActor(std::static_pointer_cast<Actor>(pMySql)) == false)
	{
		NET_ENGINE_LOG_ERROR("MySqlService - RegisterActor failed, MySQL disabled");
		return;
	}

	mMySqlActorId = pMySql->GetId();
	mbInitialized = true;
}

MySqlActorRef MySqlService::GetMySqlActorRef()
{
	MySqlActorRef pMySql = std::static_pointer_cast<MySqlActor>(mpActorService->GetActorRef(mMySqlActorId));
	return pMySql;
}
