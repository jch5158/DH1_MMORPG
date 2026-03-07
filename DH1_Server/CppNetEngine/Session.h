#pragma once
#include "Connector.h"
#include "Disconnector.h"
#include "IocpCore.h"
#include "IocpEvent.h"
#include "LockFreeQueue.h"
#include "NetAddress.h"
#include "NetReceiveBuffer.h"
#include "Receiver.h"
#include "Sender.h"
#include "SessionTimeoutTracker.h"
#include "SharedPtrUtils.h"

enum class eSessionState : uint8
{
	Connected,
	Waiting,    // 대기열 상태
	InGame,
	Disconnected
};

// 1. 끊김 사유를 명확히 정의
enum class eDisconnectReason : uint16  // NOLINT(performance-enum-size)
{
	ClientRequest,  // 클라이언트가 정상 종료
	Timeout,        // 하트비트 응답 없음
	Kicked,         // 서버에서 강퇴
	ServerFull,		// 대기큐마저 꽉 찼을 때
	ServerShutdown, // 서버 종료
	SocketError,     // 네트워크 에러
	StateError,
	ServiceError,
};

class Session : public IocpObject
{
public:

	friend class Listener;
	friend class Service;
	friend class SessionManager;
	friend class SessionReaper;
	friend class Sender;
	friend class Acceptor;
	friend class Connector;
	friend class Disconnector;
	friend class Receiver;
	friend class Sender;

	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;
	Session(Session&&) = delete;
	Session& operator=(Session&&) = delete;

	explicit Session();
	virtual ~Session() override = default;

	[[nodiscard]] virtual HANDLE GetHandle() const override;
	virtual void Dispatch(class IocpEvent& iocpEvent, const uint32 numOfBytes) override;

	virtual void OnConnected() = 0;
	virtual void OnEnterWaitQueue(const uint64 myTicket) = 0;
	virtual void OnDisconnecting(const eDisconnectReason reason) = 0;
	virtual void OnDisconnected() = 0;
	virtual void OnSend(const int32 len) = 0;
	virtual int32 OnReceive(byte* pBuffer, const int32 len) = 0;
	virtual void OnError(const int32 errorCode) = 0;

	[[nodiscard]] bool SetSessionInGame();

	[[nodiscard]] ServiceRef GetService() const;
	[[nodiscard]] SOCKET GetSocket() const;
	[[nodiscard]] NetAddress& GetAddress();
	[[nodiscard]] byte* GetReceiveBufferPtr() const;
	[[nodiscard]] SessionRef GetSessionRef();
	
	[[nodiscard]] bool IsInGame() const;
	[[nodiscard]] bool IsConnected() const;
	[[nodiscard]] bool IsWaiting() const;
	[[nodiscard]] bool IsDisconnected() const;
	[[nodiscard]] bool Connect();
	void Disconnect(const eDisconnectReason reason);
	void Send(const NetSendBufferRef& pSendBuffer);
	void Clear();

protected:

	void updateLastActivityMs();
	int64 getLastActivityMs() const;

private:

	bool registerConnect();
	void registerDisconnect();
	void registerSend();
	void registerReceive();
	void registerReap();

	void processConnect();
	void processDisconnect();
	void processSend(const uint32 numOfBytes);
	void processReceive(const uint32 numOfBytes);

	void setService(const ServiceRef& pService);
	void setSessionEvent(const ServiceRef& pService);
	void setNetAddress(const NetAddress& address);

	bool setSessionWaiting();
	bool setWaitingToConnected();
	bool setSessionConnected();
	bool setSessionDisconnected();

	ServiceRef mpService;
	SOCKET mSocket;
	NetAddress mNetAddress;
	std::atomic<eSessionState> mSessionState;
	SessionTimeoutTracker mTimeoutTracker;
	Connector mConnector;
	Disconnector mDisconnector;
	Receiver mReceiver;
	Sender mSender;
};

