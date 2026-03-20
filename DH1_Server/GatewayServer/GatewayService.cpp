#include "pch.h"
#include "GatewayService.h"

bool GatewayService::Initialize(ServerServiceRef pService, RedisServiceRef pRedisService)
{
	if (pService == nullptr || pRedisService == nullptr)
	{
		return false;
	}

	mpServerService = std::move(pService);
	mpRedisService = std::move(pRedisService);
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
