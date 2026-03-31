#pragma once

class JsonConfig;

enum class eWorldServerStatus : uint8
{
	Online = 0,
	Maintenance = 1,
	Full = 2,
	ShuttingDown = 3,
};

class WorldService : public ISingleton<WorldService>
{
public:

	template <typename T>
	friend class ISingleton;

	WorldService(const WorldService&) = delete;
	WorldService& operator=(const WorldService&) = delete;
	WorldService(WorldService&&) = delete;
	WorldService& operator=(WorldService&&) = delete;

	~WorldService() = default;

	static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType);

	[[nodiscard]] bool Initialize(const JsonConfig& config);
	[[nodiscard]] bool Start();
	void Run();
	void Stop();

	[[nodiscard]] ServerServiceRef GetServerServiceRef() const;
	[[nodiscard]] ClientServiceRef GetRealmClientServiceRef() const;
	[[nodiscard]] ActorServiceRef GetActorServiceRef() const;
	[[nodiscard]] RedisServiceRef GetRedisServiceRef() const;
	[[nodiscard]] MySqlServiceRef GetMySqlServiceRef() const;
	[[nodiscard]] int32 GetWorldServerId() const;
	[[nodiscard]] GameSessionManagerRef GetGameSessionManagerRef() const;
	[[nodiscard]] GridManagerRef GetGridManagerRef() const;
	[[nodiscard]] GameTickProcessorRef GetGameTickProcessorRef() const;
	[[nodiscard]] NavMeshManagerRef GetNavMeshManagerRef() const;
	[[nodiscard]] float GetDefaultMoveSpeed() const;

private:

	WorldService()
		: mpServerService()
		, mpRealmClientService()
		, mpActorService()
		, mpRedisService()
		, mNetworkDispatchThreadCount(0)
		, mActorDispatchThreadCount(0)
		, mWorldServerId(0)
		, mHeartbeatIntervalMs(5000)
		, mGatewayHeartbeatTimeoutMs(15000)
		, mRealmHeartbeatTimeoutMs(15000)
		, mRedisTtlSeconds(10)
		, mMaxPlayers(1000)
		, mGameTickIntervalMs(50)
		, mDefaultMoveSpeed(600.0f)
		, mAoiCellSize(10000.0f)
		, mAoiRange(1)
		, mListenPort(0)
		, mbRunning(false)
	{}

	void updateWorldRegistration(eWorldServerStatus status);
	void sendHeartbeatToRealm();
	void sendHeartbeatToGateways();
	void checkGatewayHeartbeats();
	void checkRealmHeartbeat();
	void cleanupPendingSessions();

	ServerServiceRef mpServerService;
	ClientServiceRef mpRealmClientService;
	ActorServiceRef mpActorService;
	RedisServiceRef mpRedisService;
	MySqlServiceRef mpMySqlService;
	GameSessionManagerRef mpGameSessionManager;
	GridManagerRef mpGridManager;
	NavMeshManagerRef mpNavMeshManager;
	GameTickProcessorRef mpGameTickProcessor;

	int32 mNetworkDispatchThreadCount;
	int32 mActorDispatchThreadCount;

	int32 mWorldServerId;
	std::string mWorldName;
	int64 mHeartbeatIntervalMs;
	int64 mGatewayHeartbeatTimeoutMs;
	int64 mRealmHeartbeatTimeoutMs;
	int64 mRedisTtlSeconds;
	int32 mMaxPlayers;
	int64 mGameTickIntervalMs;
	float mDefaultMoveSpeed;
	float mAoiCellSize;
	int32 mAoiRange;
	std::string mNavMeshFilePath;
	std::string mNavMeshCacheFilePath;
	std::string mListenIp;
	uint16 mListenPort;

	std::atomic<bool> mbRunning;
};
