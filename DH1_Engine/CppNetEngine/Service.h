#pragma once

#include "Listener.h"
#include "NetAddress.h"
#include "LockFreeStack.h"
#include "NetworkScheduler.h"
#include "SessionManager.h"
#include "WaitQueueManager.h"

enum class eServiceType : uint8
{
	Server,
	Client
};

using SessionFactory = std::function<SessionRef()>;

class Service : public std::enable_shared_from_this<Service>
{
public:

	Service(const Service&) = delete;
	Service& operator=(const Service&) = delete;
	Service(Service&&) = delete;
	Service& operator=(Service&&) = delete;

	explicit Service(
		const eServiceType serviceType,
		const NetAddress& netAddress,
		const int32 maxSessionCount,
		NetworkSchedulerRef pNetworkScheduler,
		SessionFactory sessionFactory);
	virtual ~Service() = default;

	virtual bool Start() = 0;
	virtual void CloseService() = 0;
	virtual bool AddSession(const SessionRef& pSession) = 0;
	virtual void RemoveSession(const SessionRef& pSession) = 0;

	SessionRef CreateSession();

	[[nodiscard]] eServiceType GetServiceType() const;
	[[nodiscard]] NetAddress& GetNetAddress();
	[[nodiscard]] NetworkSchedulerRef GetNetworkScheduler() const;
	[[nodiscard]] int32 GetCurrentSessionCount();
	[[nodiscard]] int32 GetMaxSessionCount() const;

protected:
	
	const eServiceType mServiceType;
	const int32	mMaxSessionCount;
	NetAddress mNetAddress;
	NetworkSchedulerRef mpNetworkScheduler;
	SessionFactory mSessionFactory;
	SessionManager mSessionManager;
};

struct ClientServiceConfig
{
	NetAddress netAddress;
	int32 maxSessionCount;
	NetworkSchedulerRef pNetworkScheduler;
	SessionFactory sessionFactory;
};

class ClientService : public Service
{
public:
	explicit ClientService(const ClientServiceConfig& config);
	virtual ~ClientService() override = default;

	virtual bool Start() override;
	virtual void CloseService() override;
	virtual bool AddSession(const SessionRef& pSession) override;
	virtual void RemoveSession(const SessionRef& pSession) override;
};

struct ServerServiceConfig
{
	NetAddress netAddress;
	int32 acceptCount;
	int32 maxSessionCount;
	int32 maxWaitSessionCount;
	int64 sessionTimeoutMs;
	NetworkSchedulerRef pNetworkScheduler;
	SessionFactory sessionFactory;
};

class ServerService : public Service
{
public:
	explicit ServerService(const ServerServiceConfig& config);
	virtual ~ServerService() override = default;

	virtual bool Start() override;
	virtual void CloseService() override;	
	virtual bool AddSession(const SessionRef& pSession) override;
	virtual void RemoveSession(const SessionRef& pSession) override;

	[[nodiscard]] bool EnterWaitQueue(const SessionRef& pSession, uint64& outTicket);
	[[nodiscard]] SessionRef DequeueWaitQueue();
	void RegisterSessionReap(const SessionRef& pSession);
	void RegisterAbortSession(const SessionRef& pSession);

	[[nodiscard]] uint64 GetWaitCount(const uint64 myTicket);

private:

	void admitWaitingSession();
	
	ListenerRef mpListener;
	WaitQueueManager mWaitQueueManager;
	SessionReaperRef mpSessionReaper;
};