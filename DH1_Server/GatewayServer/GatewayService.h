#pragma once
#include <Service.h>
#include "ClientSessionManager.h"
#include "RedisService.h"

struct GatewayServiceConfig
{
	ServerServiceConfig serviceConfig;
};

class GatewayService : public ISingleton<GatewayService>
{
public:

	template <typename T>
	friend class ISingleton;

	GatewayService(const GatewayService&) = delete;
	GatewayService& operator=(const GatewayService&) = delete;
	GatewayService(GatewayService&&) = delete;
	GatewayService& operator=(GatewayService&&) = delete;

	~GatewayService() = default;

	[[nodiscard]] bool Initialize(ServerServiceRef pService, RedisServiceRef pRedisService);

	[[nodiscard]] ClientSessionManagerRef GetClientSessionManagerRef() const;
	[[nodiscard]] RedisServiceRef GetRedisServiceRef() const;

private:

	GatewayService()
		:mpClientSessionManager()
		, mpServerService()
		, mpRedisService()
	{}

	ClientSessionManagerRef mpClientSessionManager;
	ServerServiceRef mpServerService;
	RedisServiceRef mpRedisService;
};

