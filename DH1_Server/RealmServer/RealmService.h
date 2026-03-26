#pragma once

class JsonConfig;
class RedisService;
using RedisServiceRef = std::shared_ptr<RedisService>;

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
	[[nodiscard]] RealmSessionRegistryRef GetSessionRegistryRef() const;

private:

	RealmService()
		: mpServerService()
		, mpActorService()
		, mNetworkDispatchThreadCount(0)
		, mActorDispatchThreadCount(0)
		, mRealmServerId(0)
		, mMaxPlayers(0)
		, mRedisTtlSeconds(10)
		, mHeartbeatTimeoutMs(15000)
		, mHeartbeatCheckIntervalMs(5000)
		, mHeartbeatIntervalMs(5000)
		, mListenPort(0)
		, mbRunning(false)
	{}

	void checkWorldServerHeartbeats();
	void sendHeartbeatToWorldServers();
	void updateRealmRegistration();

	ServerServiceRef mpServerService;
	ActorServiceRef mpActorService;
	RealmSessionRegistryRef mpSessionRegistry;
	RedisServiceRef mpRedisService;

	int32 mNetworkDispatchThreadCount;
	int32 mActorDispatchThreadCount;

	int32 mRealmServerId;
	std::string mRealmName;
	int32 mMaxPlayers;
	int64 mRedisTtlSeconds;
	int64 mHeartbeatTimeoutMs;
	int64 mHeartbeatCheckIntervalMs;
	int64 mHeartbeatIntervalMs;
	std::string mListenIp;
	uint16 mListenPort;

	std::atomic<bool> mbRunning;
};
