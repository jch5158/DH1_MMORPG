#pragma once
#include "NetAddress.h"
#include "SessionManager.h"
#include "SharedPtrUtils.h"
#include "Types.h"

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
	virtual void OnSessionConnected(const SessionRef& pSession) = 0;
	virtual void OnSessionDisconnected(const SessionRef& pSession) = 0;

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
	virtual void OnSessionConnected(const SessionRef& pSession) override;
	virtual void OnSessionDisconnected(const SessionRef& pSession) override;

private:

	ConnectionManagerRef mpConnectionManager;
};

struct ServerServiceConfig
{
	NetAddress netAddress;
	int32 acceptCount;
	int32 maxSessionCount;
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
	virtual void OnSessionConnected(const SessionRef& pSession) override;
	virtual void OnSessionDisconnected(const SessionRef& pSession) override;

	void RegisterSessionReap(const SessionRef& pSession);
	void RegisterAbortSession(const SessionRef& pSession);

private:
		
	ListenerRef mpListener;
	SessionReaperRef mpSessionReaper;
};