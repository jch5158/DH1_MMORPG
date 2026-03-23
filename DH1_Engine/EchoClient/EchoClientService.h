#pragma once

class JsonConfig;

class EchoClientService : public ISingleton<EchoClientService>
{
public:

	template <typename T>
	friend class ISingleton;

	EchoClientService(const EchoClientService&) = delete;
	EchoClientService& operator=(const EchoClientService&) = delete;
	EchoClientService(EchoClientService&&) = delete;
	EchoClientService& operator=(EchoClientService&&) = delete;

	~EchoClientService() = default;

	[[nodiscard]] bool Initialize(const JsonConfig& config);
	[[nodiscard]] bool Start();
	void Run();
	void Stop();

	[[nodiscard]] ClientServiceRef GetClientServiceRef() const;

private:

	EchoClientService()
		: mpClientService()
		, mNetworkDispatchThreadCount(0)
		, mbRunning(false)
	{}

	ClientServiceRef mpClientService;
	int32 mNetworkDispatchThreadCount;
	std::atomic<bool> mbRunning;
};
