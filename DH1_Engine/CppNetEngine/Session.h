#pragma once

enum class eSessionState : uint8
{
	None,
	Connected,
	InGame,
	Disconnecting,
	Disconnected
};

enum class eDisconnectReason : uint8  // NOLINT(performance-enum-size)
{
	Closed,			 // 클라이언트가 정상 종료
	DuplicateLogin,  // 중복 접속
	Timeout,         // 하트비트 응답 없음
	Kicked,          // 서버에서 강퇴
	ServerFull,		 // 대기큐마저 꽉 찼을 때
	ServerShutdown,  // 서버 종료
	SocketError,     // 네트워크 에러
	StateError,
	ServiceError,
};

class Session : public SocketIocpObject
{
public:

	friend class Listener;
	friend class NetService;
	friend class ClientService;
	friend class ServerService;
	friend class SessionManager;
	friend class SessionReaper;
	friend class Sender;
	friend class Acceptor;
	friend class Connector;
	friend class Disconnector;
	friend class Receiver;
	
	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;
	Session(Session&&) = delete;
	Session& operator=(Session&&) = delete;

	explicit Session(const int32 receiveBufferSize);
	virtual ~Session() override;

	virtual void Dispatch(class IocpEvent& iocpEvent, const uint32 numOfBytes) override;

	virtual void OnConnected() = 0;
	virtual void OnDisconnecting(const eDisconnectReason reason) = 0;
	virtual void OnDisconnected() = 0;
	virtual void OnSend(const int32 len) = 0;
	virtual int32 OnReceive(const byte* pBuffer, const int32 len) = 0;

	void Start();
	void Stop();

	[[nodiscard]] NetServiceRef GetService() const;
	[[nodiscard]] NetAddress& GetAddress();
	[[nodiscard]] SessionRef GetSessionRef();
	[[nodiscard]] uint64 GetSessionId() const;

	[[nodiscard]] bool IsInGame() const;
	[[nodiscard]] bool IsConnected() const;
	[[nodiscard]] bool IsDisconnecting() const;
	[[nodiscard]] bool IsDisconnected() const;
	[[nodiscard]] bool IsDisconnectStarted() const;
	[[nodiscard]] bool SetSessionInGame();
	void Disconnect(const eDisconnectReason reason);
	void Send(const NetSendBufferRef& pSendBuffer);

protected:

	void updateLastActivityMs();
	int64 getLastActivityMs() const;

private:

	void registerDisconnect();
	void registerSend();
	void registerReceive();

	void processDisconnect() const;
	void processSend(const uint32 numOfBytes);
	void processReceive(const uint32 numOfBytes);

	bool Initialize(const NetServiceRef& pService);
	void setNetAddress(const NetAddress& address);
	void startReceive();

	bool setSessionInitialized();
	bool setSessionConnected();
	bool setSessionDisconnecting();
	bool setSessionDisconnected();
	void setSessionNone();

	uint64 mSessionId;
	NetServiceRef mpService;
	NetAddress mNetAddress;
	std::atomic<eSessionState> mSessionState;
	SessionTimeoutTracker mTimeoutTracker;
	Disconnector mDisconnector;
	Receiver mReceiver;
	Sender mSender;
};

