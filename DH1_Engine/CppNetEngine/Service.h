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

class Service : public std::enable_shared_from_this<Service>
{
public:

	using SessionFactory = std::function<SessionRef()>;

	Service(const Service&) = delete;
	Service& operator=(const Service&) = delete;
	Service(Service&&) = delete;
	Service& operator=(Service&&) = delete;

	explicit Service(
		const eServiceType serviceType,
		const NetAddress& netAddress,
		SessionFactory sessionFactory,
		IocpCoreRef pIocpCore,
		SessionReaperRef pSessionReaper,
		SessionManagerRef pSessionManager,
		WaitQueueManagerRef pWaitQueueManager);
	virtual ~Service() = default;

	virtual bool Start() = 0;
	virtual void CloseService();

	SessionRef CreateSession();
	[[nodiscard]] bool AddSession(const SessionRef& pSession) const;
	void RemoveSession(const SessionRef& pSession) const;
	[[nodiscard]] bool EnterWaitQueue(const SessionRef& pSession, uint64& outTicket) const;
	[[nodiscard]] SessionRef DequeueWaitQueue() const;
	void RegisterSessionReap(const SessionRef& pSession) const;

	[[nodiscard]] eServiceType GetServiceType() const;
	[[nodiscard]] NetAddress& GetNetAddress();
	[[nodiscard]] IocpCoreRef GetIocpCore() const;
	[[nodiscard]] int32 GetCurrentSessionCount() const;
	[[nodiscard]] int32 GetMaxSessionCount() const;
	[[nodiscard]] bool GetWaitCount(const uint64 myTicket, uint64& outWaitCount) const;

private:

	void admitWaitingSession() const;

	const eServiceType mServiceType;
	const int32	mMaxSessionCount;
	NetAddress mNetAddress;
	SessionFactory mSessionFactory;
	IocpCoreRef mpIocpCore;
	SessionReaperRef mpSessionReaper;
	SessionManagerRef mpSessionManager;
	WaitQueueManagerRef mpWaitQueueManager;
};

class ClientService : public Service
{
public:
	explicit ClientService(
		const NetAddress& targetAddress, 
		SessionFactory pSessionFactory,
		NetworkSchedulerRef pNetworkScheduler,
		SessionManagerRef pSessionManager);
	virtual ~ClientService() override = default;

	virtual bool Start() override;
	virtual void CloseService() override;
};

class ServerService : public Service
{
public:
	explicit ServerService(
		const NetAddress& targetAddress,
		SessionFactory pSessionFactory,
		ListenerRef pListener,
		NetworkSchedulerRef pNetworkScheduler,
		SessionReaperRef pSessionReaper,
		SessionManagerRef pSessionManager,
		WaitQueueManagerRef pWaitQueueManager);
	virtual ~ServerService() override = default;

	virtual bool Start() override;
	virtual void CloseService() override;

private:
	ListenerRef mpListener;
};