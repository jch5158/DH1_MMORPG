#pragma once

#include "RedisService.h"

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

