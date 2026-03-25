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

	static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType);

	[[nodiscard]] bool Initialize(const JsonConfig& config);
	[[nodiscard]] bool Start();
	void Run();
	void Stop();

	[[nodiscard]] ServerServiceRef GetServerServiceRef() const;
	[[nodiscard]] ClientServiceRef GetWorldClientServiceRef() const;
	[[nodiscard]] ActorServiceRef GetActorServiceRef() const;
	[[nodiscard]] ClientSessionManagerRef GetClientSessionManagerRef() const;
	[[nodiscard]] RedisServiceRef GetRedisServiceRef() const;
	[[nodiscard]] MySqlServiceRef GetMySqlServiceRef() const;
	[[nodiscard]] MySqlServiceRef GetAccountMySqlServiceRef() const;
	[[nodiscard]] int32 GetGatewayId() const;

private:

	GatewayService()
		: mpServerService()
		, mpWorldClientService()
		, mpActorService()
		, mpRedisService()
		, mpClientSessionManager()
		, mNetworkDispatchThreadCount(0)
		, mActorDispatchThreadCount(0)
		, mGatewayId(0)
		, mHeartbeatIntervalMs(5000)
		, mClientHeartbeatTimeoutMs(15000)
		, mWorldHeartbeatTimeoutMs(15000)
		, mRedisTtlSeconds(10)
		, mListenPort(0)
		, mbRunning(false)
	{}

	void updateGatewayRegistration(eGatewayStatus status);
	void checkClientHeartbeats();
	void checkWorldHeartbeats();
	void sendHeartbeatToWorld();

	ServerServiceRef mpServerService;
	ClientServiceRef mpWorldClientService;
	ActorServiceRef mpActorService;
	RedisServiceRef mpRedisService;
	MySqlServiceRef mpMySqlService;
	MySqlServiceRef mpAccountMySqlService;
	ClientSessionManagerRef mpClientSessionManager;

	int32 mNetworkDispatchThreadCount;
	int32 mActorDispatchThreadCount;

	int32 mGatewayId;
	int64 mHeartbeatIntervalMs;
	int64 mClientHeartbeatTimeoutMs;
	int64 mWorldHeartbeatTimeoutMs;
	int64 mRedisTtlSeconds;
	std::string mListenIp;
	std::string mPublicIp;
	uint16 mListenPort;

	std::atomic<bool> mbRunning;
};
