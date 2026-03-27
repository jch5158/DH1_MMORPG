#pragma once

#include "RedisService.h"

class JsonConfig;

enum class eRealmServerStatus : uint8
{
	Online = 0,
	Maintenance = 1,
	ShuttingDown = 2,
};

class RealmService : public ISingleton<RealmService>
{
public:

	template <typename T>
	friend class ISingleton;

	RealmService(const RealmService&) = delete;
	RealmService& operator=(const RealmService&) = delete;
	RealmService(RealmService&&) = delete;
	RealmService& operator=(RealmService&&) = delete;

	~RealmService() = default;

	static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType);

	[[nodiscard]] bool Initialize(const JsonConfig& config);
	[[nodiscard]] bool Start();
	void Run();
	void Stop();

	[[nodiscard]] ServerServiceRef GetServerServiceRef() const;
	[[nodiscard]] ActorServiceRef GetActorServiceRef() const;
	[[nodiscard]] RedisServiceRef GetRedisServiceRef() const;
	[[nodiscard]] RealmSessionRegistryRef GetSessionRegistryRef() const;

private:

	RealmService()
		: mpServerService()
		, mpActorService()
		, mpRedisService()
		, mNetworkDispatchThreadCount(0)
		, mActorDispatchThreadCount(0)
		, mRealmServerId(0)
		, mHeartbeatTimeoutMs(15000)
		, mHeartbeatCheckIntervalMs(5000)
		, mHeartbeatIntervalMs(5000)
		, mRedisTtlSeconds(10)
		, mMaxPlayers(5000)
		, mListenPort(0)
		, mbRunning(false)
	{}

	void checkWorldServerHeartbeats();
	void sendHeartbeatToWorldServers();
	void updateRealmRegistration();

	ServerServiceRef mpServerService;
	ActorServiceRef mpActorService;
	RedisServiceRef mpRedisService;
	RealmSessionRegistryRef mpSessionRegistry;

	int32 mNetworkDispatchThreadCount;
	int32 mActorDispatchThreadCount;

	int32 mRealmServerId;
	int64 mHeartbeatTimeoutMs;
	int64 mHeartbeatCheckIntervalMs;
	int64 mHeartbeatIntervalMs;
	int64 mRedisTtlSeconds;
	int32 mMaxPlayers;
	std::string mRealmName;
	std::string mListenIp;
	uint16 mListenPort;

	std::atomic<bool> mbRunning;
};
