#include "pch.h"
#include "GatewayService.h"
#include "ClientSessionManager.h"

bool GatewayService::Initialize(ServerServiceRef pService, RedisServiceRef pRedisService)
{
	if (pService == nullptr || pRedisService == nullptr)
	{
		return false;
	}

	mpServerService = std::move(pService);
	mpRedisService = std::move(pRedisService);
	mpClientSessionManager = cpp_net_engine::MakeShared<ClientSessionManager>(1);
	return true;
}

ClientSessionManagerRef GatewayService::GetClientSessionManagerRef() const
{
	return mpClientSessionManager;
}

RedisServiceRef GatewayService::GetRedisServiceRef() const
{
	return mpRedisService;
}
