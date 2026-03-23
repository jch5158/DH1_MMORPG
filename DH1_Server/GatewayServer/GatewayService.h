#pragma once

#include "RedisService.h"

class JsonConfig;

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

	[[nodiscard]] bool Initialize(const JsonConfig& config);
	[[nodiscard]] bool Start();
	void Run();
	void Stop();

	[[nodiscard]] ServerServiceRef GetServerServiceRef() const;
	[[nodiscard]] ActorServiceRef GetActorServiceRef() const;
	[[nodiscard]] ClientSessionManagerRef GetClientSessionManagerRef() const;
	[[nodiscard]] RedisServiceRef GetRedisServiceRef() const;

private:

	GatewayService()
		: mpServerService()
		, mpActorService()
		, mpRedisService()
		, mpClientSessionManager()
		, mbRunning(false)
	{}

	ServerServiceRef mpServerService;
	ActorServiceRef mpActorService;
	RedisServiceRef mpRedisService;
	ClientSessionManagerRef mpClientSessionManager;

	int32 mNetworkDispatchThreadCount = 0;
	int32 mActorDispatchThreadCount = 0;

	std::atomic<bool> mbRunning;
};
