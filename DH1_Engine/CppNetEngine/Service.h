#pragma once

#include "Listener.h"
#include "NetAddress.h"
#include "LockFreeStack.h"
#include "SessionManager.h"
#include "WaitQueueManager.h"

enum class eServiceType : uint8
{
	Server,
	Client
};

class Service : public std::enable_shared_from_this<Service>
{
public:

	using SessionFactory = std::function<SessionRef()>;

	Service(const Service&) = delete;
	Service& operator=(const Service&) = delete;
	Service(Service&&) = delete;
	Service& operator=(Service&&) = delete;

	explicit Service(const eServiceType serviceType, const NetAddress& netAddress, IocpCoreRef pIocpCore, ActorSchedulerRef pScheduler, SessionFactory pSessionFactory, SessionManagerRef pSessionManager, SessionReaperRef pSessionReaper, WaitQueueManagerRef pWaitQueueManager);
	virtual ~Service() = default;

	virtual bool Start() = 0;
	virtual void CloseService();

	SessionRef CreateSession();
	bool AddSession(const SessionRef& pSession) const;
	void RemoveSession(const SessionRef& pSession) const;
	bool EnterWaitQueue(const SessionRef& pSession, uint64& outTicket) const;
	SessionRef DequeueWaitQueue() const;
	void RegisterSessionReap(const SessionRef& pSession) const;

	eServiceType GetServiceType() const;
	NetAddress& GetNetAddress();
	IocpCoreRef GetIocpCore() const;
	ActorSchedulerRef GetActorScheduler() const;
	int32 GetCurrentSessionCount() const;
	int32 GetMaxSessionCount() const;
	bool GetWaitCount(const uint64 myTicket, uint64& outWaitCount) const;

private:

	void admitWaitingSession() const;

	const eServiceType mServiceType;
	const int32	mMaxSessionCount;
	NetAddress mNetAddress;
	IocpCoreRef mpIocpCore;
	ActorSchedulerRef mpScheduler;
	SessionFactory mpSessionFactory;
	SessionManagerRef mpSessionManager;
	SessionReaperRef mpSessionReaper;
	WaitQueueManagerRef mpWaitQueueManager;
};

class ClientService : public Service
{
public:
	explicit ClientService(const NetAddress& targetAddress, IocpCoreRef pIocpCore, ActorSchedulerRef pScheduler, SessionFactory pSessionFactory, SessionManagerRef pSessionManager);
	virtual ~ClientService() override = default;

	virtual bool Start() override;
	virtual void CloseService() override;
};

class ServerService : public Service
{
public:
	explicit ServerService(const NetAddress& targetAddress, ListenerRef pListener, IocpCoreRef pIocpCore, ActorSchedulerRef pScheduler, SessionFactory pSessionFactory, SessionManagerRef pSessionManager, SessionReaperRef pSessionReaper, WaitQueueManagerRef pWaitQueueManager);
	virtual ~ServerService() override = default;

	virtual bool Start() override;
	virtual void CloseService() override;

private:
	ListenerRef mpListener;
};