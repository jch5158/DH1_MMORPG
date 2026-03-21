#include "pch.h"
#include "Session.h"

Session::Session(const int32 receiveBufferSize)
	: SocketIocpObject()
	, mpService()
	, mNetAddress()
	, mSessionState(eSessionState::Disconnected)
	, mTimeoutTracker()
	, mDisconnector()
	, mReceiver(receiveBufferSize)
	, mSender()
{
}

Session::~Session()
{
	ForceCloseSocket();
}

void Session::Dispatch(IocpEvent& iocpEvent, const uint32 numOfBytes)
{
	switch (iocpEvent.GetEventType())  // NOLINT(clang-diagnostic-switch-enum)
	{
	case eIocpEventType::Disconnect:
		processDisconnect();
		break;
	case eIocpEventType::Send:
		processSend(numOfBytes);
		break;
	case eIocpEventType::Receive:
		processReceive(numOfBytes);
		break;
	default:
		NET_ENGINE_LOG_ERROR("Session::Dispatch - iocp event type is unmatched, iocpEvent.GetEventType() : {}", static_cast<uint8>(iocpEvent.GetEventType()));
		break;
	}
}

void Session::Start()
{
	updateLastActivityMs();

	registerReceive();

	OnConnected();
}

void Session::Stop()
{
	OnDisconnected();
}

ServiceRef Session::GetService() const
{
	return mpService;
}

NetAddress& Session::GetAddress()
{
	return mNetAddress;
}

byte* Session::GetReceiveBufferPtr() const
{
	return mReceiver.GetWritePtr();
}

SessionRef Session::GetSessionRef()
{
	return std::static_pointer_cast<Session>(shared_from_this());
}

bool Session::IsInGame() const
{
	return mSessionState.load() == eSessionState::InGame;
}

bool Session::IsConnected() const
{
	return mSessionState.load() == eSessionState::Connected;
}

bool Session::IsDisconnecting() const
{
	return mSessionState.load() == eSessionState::Disconnecting;
}

bool Session::IsDisconnected() const
{
	return mSessionState.load() == eSessionState::Disconnected;
}

bool Session::IsDisconnectStarted() const
{
	return mSessionState.load() == eSessionState::Disconnecting || mSessionState.load() == eSessionState::Disconnected;
}

bool Session::SetSessionInGame()
{
	auto expected = eSessionState::Connected;
	return mSessionState.compare_exchange_weak(expected, eSessionState::InGame);
}

void Session::Disconnect(const eDisconnectReason reason)
{
	if (!setSessionDisconnecting())
	{
		return;
	}

	OnDisconnecting(reason);
	registerDisconnect();
}

void Session::Send(const NetSendBufferRef& pSendBuffer)
{
	mSender.Send(pSendBuffer);
}

void Session::updateLastActivityMs()
{
	mTimeoutTracker.UpdateLastActivityMs();
}

int64 Session::getLastActivityMs() const
{
	return mTimeoutTracker.GetLastActivityMs();
}

void Session::registerDisconnect()
{
	mDisconnector.Register();
}

void Session::registerSend()
{
	mSender.Register();
}

void Session::registerReceive()
{
	mReceiver.Register();
}

void Session::processDisconnect() const
{
	mDisconnector.Process();
}

void Session::processSend(const uint32 numOfBytes)
{
	mSender.Process(numOfBytes);
}

void Session::processReceive(const uint32 numOfBytes)
{
	mReceiver.Process(numOfBytes);
}

bool Session::Initialize(const ServiceRef& pService)
{
	if (CreateSocket() == false)
	{
		return false;
	}

	mpService = pService;
	const SessionRef pSession = GetSessionRef();

	if (mDisconnector.Initialize(pSession, pService) == false)
	{
		return false;
	}
	
	if (mReceiver.Initialize(pSession) == false)
	{
		return false;
	}

	if (mSender.Initialize(pSession) == false)
	{
		return false;
	}

	return true;
}

void Session::setNetAddress(const NetAddress& address)
{
	mNetAddress = address;
}

void Session::startReceive()
{
	updateLastActivityMs();
	registerReceive();
}

bool Session::setSessionConnected()
{
	auto expected = eSessionState::Disconnected;
	return mSessionState.compare_exchange_strong(expected, eSessionState::Connected);
}

bool Session::setSessionDisconnecting()
{
	eSessionState expected = mSessionState.load();

	while (expected == eSessionState::Connected ||
		expected == eSessionState::InGame)
	{
		if (mSessionState.compare_exchange_strong(expected, eSessionState::Disconnecting))
		{
			return true;
		}
	}

	return false;
}

bool Session::setSessionDisconnected()
{
	auto expected = eSessionState::Disconnecting;
	return mSessionState.compare_exchange_strong(expected, eSessionState::Disconnected);
}