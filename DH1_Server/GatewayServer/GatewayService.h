#pragma once
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
	[[nodiscard]] RedisServiceRef GetRedisServiceRef() const;

private:

	GatewayService() = default;

	ServerServiceRef mpServerService;
	RedisServiceRef mpRedisService;
};

