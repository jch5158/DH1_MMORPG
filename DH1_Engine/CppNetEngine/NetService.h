#pragma once

enum class eServiceType : uint8
{
	Server,
	Client
};

using SessionFactory = std::function<SessionRef()>;

struct NetServiceConfig
{
	virtual ~NetServiceConfig() = default;

	int32 maxSessionCount;
	NetAddress netAddress;
	NetworkSchedulerRef pNetworkScheduler;
	SessionFactory sessionFactory;
	std::string redisConnectionUri;
};

class NetService : public std::enable_shared_from_this<NetService>
{
public:

	NetService(const NetService&) = delete;
	NetService& operator=(const NetService&) = delete;
	NetService(NetService&&) = delete;
	NetService& operator=(NetService&&) = delete;

	explicit NetService(const eServiceType serviceType);
	virtual ~NetService() = default;

	virtual bool Initialize(const NetServiceConfig& config);

	virtual bool Start() = 0;
	virtual void CloseService() = 0;
	virtual void OnSessionConnected(const SessionRef&) = 0;
	virtual void OnSessionDisconnected(const SessionRef&) = 0;
	virtual void Dispatch();

	SessionRef CreateSession();

	[[nodiscard]] bool IsInitialized() const;
	[[nodiscard]] eServiceType GetServiceType() const;
	[[nodiscard]] NetAddress& GetNetAddress();
	[[nodiscard]] NetworkSchedulerRef GetNetworkScheduler() const;
	[[nodiscard]] int32 GetCurrentSessionCount();
	[[nodiscard]] int32 GetMaxSessionCount() const;
	uint64 AllocateSessionId();

private:
	std::atomic<bool> mbInitialize;

protected:
	const eServiceType mServiceType;
	NetAddress mNetAddress;
	NetworkSchedulerRef mpNetworkScheduler;
	SessionFactory mSessionFactory;
	SessionManager mSessionManager;
	RedisConnectionRef mpRedisConnection;
	SessionIdAllocatorRef mpSessionIdAllocator;
};

struct ClientServiceConfig : public NetServiceConfig
{
	virtual ~ClientServiceConfig() override = default;
	int32 maxConnectionCount;
	bool bAutoReconnect = true;
	int64 reconnectIntervalMs = 3000;
	int32 maxReconnectCount = 0; // 0 = unlimited
};

class ClientService : public NetService
{
public:
	explicit ClientService(const eServiceType serviceType);
	virtual ~ClientService() override = default;

	virtual bool Initialize(const NetServiceConfig& config) override;

	virtual bool Start() override;
	virtual void CloseService() override;
	virtual void OnSessionConnected(const SessionRef& pSession) override;
	virtual void OnSessionDisconnected(const SessionRef& pSession) override;

	SessionRef GetFirstSessionRef();

private:

	void scheduleReconnect();

	ConnectionPoolRef mpConnectionManager;
	bool mbAutoReconnect;
	int64 mReconnectIntervalMs;
	int32 mMaxReconnectCount;
	std::atomic<int32> mReconnectAttemptCount;
};

struct ServerServiceConfig : public NetServiceConfig
{
	virtual ~ServerServiceConfig() override = default;
	int32 acceptCount;
	int64 sessionTimeoutMs;
};

class ServerService : public NetService
{
public:
	explicit ServerService(const eServiceType serviceType);
	virtual ~ServerService() override = default;

	virtual bool Initialize(const NetServiceConfig& config) override;

	virtual bool Start() override;
	virtual void CloseService() override;	
	virtual void OnSessionConnected(const SessionRef& pSession) override;
	virtual void OnSessionDisconnected(const SessionRef& pSession) override;

	[[nodiscard]] Vector<SessionRef> GetActiveSessions();

private:

	void registerReaperSweep();

	ListenerRef mpListener;
	SessionReaperRef mpSessionReaper;
};