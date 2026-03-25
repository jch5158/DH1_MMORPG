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

	[[nodiscard]] ClientServiceRef GetClientServiceRef() const;
	[[nodiscard]] ActorServiceRef GetActorServiceRef() const;

private:

	WorldService()
		: mpClientService()
		, mpActorService()
		, mNetworkDispatchThreadCount(0)
		, mActorDispatchThreadCount(0)
		, mWorldServerId(0)
		, mbRunning(false)
	{}

	ClientServiceRef mpClientService;
	ActorServiceRef mpActorService;

	int32 mNetworkDispatchThreadCount;
	int32 mActorDispatchThreadCount;

	int32 mWorldServerId;

	std::atomic<bool> mbRunning;
};
