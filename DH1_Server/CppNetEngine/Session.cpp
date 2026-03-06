#include "pch.h"
#include "Session.h"
#include "CrashReporter.h"
#include "Service.h"
#include "SocketUtils.h"
#include "NetSendBuffer.h"

Session::Session()
	: mpService()
	, mSocket(INVALID_SOCKET)
	, mNetAddress()
	, mSessionState(eSessionState::Disconnected)
	, mConnector()
	, mDisconnector()
	, mReceiver()
	, mSender()
{
	if (SocketUtils::CreateTcpSocket(mSocket) == false)
	{
		CrashReporter::Crash();
	}
}

HANDLE Session::GetHandle() const
{
	return reinterpret_cast<HANDLE>(mSocket);  // NOLINT(performance-no-int-to-ptr)
}

void Session::Dispatch(IocpEvent& iocpEvent, const uint32 numOfBytes)
{
	switch (iocpEvent.GetEventType())  // NOLINT(clang-diagnostic-switch-enum)
	{
	case eIocpEventType::Connect:
		processConnect();
		break;
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

bool Session::SetSessionInGame()
{
	auto expected = eSessionState::Connected;
	return mSessionState.compare_exchange_weak(expected, eSessionState::InGame);
}

ServiceRef Session::GetService() const
{
	return mpService;
}

SOCKET Session::GetSocket() const
{
	return mSocket;
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

bool Session::IsWaiting() const
{
	return mSessionState.load() == eSessionState::Waiting;
}

bool Session::IsDisconnected() const
{
	return mSessionState.load() == eSessionState::Disconnected;
}

bool Session::Connect()
{
	return registerConnect();
}

void Session::Disconnect(const eDisconnectReason reason)
{
	if (!setSessionDisconnected())
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

void Session::Clear()
{
	mpService.reset();
	mSocket = INVALID_SOCKET;
	mConnector.Clear();
	mDisconnector.Clear();
	mReceiver.Clear();
	mSender.Clear();
}

bool Session::registerConnect()
{
	return mConnector.Register();
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

void Session::registerReap()
{
	GetService()->RegisterSessionReap(GetSessionRef());
}

void Session::processConnect()
{
	mConnector.Process();

	const SessionRef pSession = GetSessionRef();
	const ServiceRef pService = GetService();

	mReceiver.SetOwner(pSession);
	mSender.SetOwner(pSession);

	if (pService->AddSession(pSession))
	{
		registerReap();
		registerReceive();
	}
	else
	{
		Disconnect(eDisconnectReason::ServerFull);
	}
}

void Session::processDisconnect()
{
	mDisconnector.Process();

	OnDisconnected();

	GetService()->RemoveSession(GetSessionRef());

	Clear();
}

void Session::processSend(const uint32 numOfBytes)
{
	mSender.Process(numOfBytes);
}

void Session::processReceive(const uint32 numOfBytes)
{
	mReceiver.Process(numOfBytes);
}

void Session::setService(const ServiceRef& pService)
{
	mpService = pService;
}

void Session::setSessionEvent(const ServiceRef& pService)
{
	mConnector.SetService(pService);

	const SessionRef pSession = GetSessionRef();
	mConnector.SetOwner(pSession);
	mDisconnector.SetOwner(pSession);
	mReceiver.SetOwner(pSession);
	mSender.SetOwner(pSession);
}

void Session::setNetAddress(const NetAddress& address)
{
	mNetAddress = address;
}

bool Session::setSessionWaiting()
{
	auto expected = eSessionState::Disconnected;
	return mSessionState.compare_exchange_weak(expected, eSessionState::Disconnected);
}

bool Session::setWaitingToConnected()
{
	auto expected = eSessionState::Waiting;
	return mSessionState.compare_exchange_weak(expected, eSessionState::Connected);
}

bool Session::setSessionConnected()
{
	auto expected = eSessionState::Disconnected;
	return mSessionState.compare_exchange_weak(expected, eSessionState::Connected);
}

bool Session::setSessionDisconnected()
{
	return mSessionState.exchange(eSessionState::Disconnected) != eSessionState::Disconnected;
}