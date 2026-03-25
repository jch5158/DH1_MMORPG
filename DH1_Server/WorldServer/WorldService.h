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
	[[nodiscard]] ActorServiceRef GetActorServiceRef() const;

private:

	WorldService()
		: mpServerService()
		, mpActorService()
		, mNetworkDispatchThreadCount(0)
		, mActorDispatchThreadCount(0)
		, mWorldServerId(0)
		, mListenPort(0)
		, mbRunning(false)
	{}

	ServerServiceRef mpServerService;
	ActorServiceRef mpActorService;

	int32 mNetworkDispatchThreadCount;
	int32 mActorDispatchThreadCount;

	int32 mWorldServerId;
	std::string mListenIp;
	uint16 mListenPort;

	std::atomic<bool> mbRunning;
};
