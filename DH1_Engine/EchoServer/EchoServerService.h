#pragma once

class JsonConfig;

class EchoServerService : public ISingleton<EchoServerService>
{
public:

	template <typename T>
	friend class ISingleton;

	EchoServerService(const EchoServerService&) = delete;
	EchoServerService& operator=(const EchoServerService&) = delete;
	EchoServerService(EchoServerService&&) = delete;
	EchoServerService& operator=(EchoServerService&&) = delete;

	~EchoServerService() = default;

	[[nodiscard]] bool Initialize(const JsonConfig& config);
	[[nodiscard]] bool Start();
	void Run();
	void Stop();

	[[nodiscard]] ServerServiceRef GetServerServiceRef() const;

private:

	EchoServerService()
		: mpServerService()
		, mNetworkDispatchThreadCount(0)
		, mbRunning(false)
	{}

	ServerServiceRef mpServerService;
	int32 mNetworkDispatchThreadCount;
	std::atomic<bool> mbRunning;
};
