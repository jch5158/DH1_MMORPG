#include "pch.h"
#include "Listener.h"

#include "Acceptor.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "ObjectAllocator.h"
#include "Service.h"
#include "Session.h"

Listener::Listener(const int32 acceptCount, std::function<void(const uint32)> pErrorHandle)
	: mSocket(INVALID_SOCKET)
	, mAcceptCount(acceptCount)
	, mpErrorHandle(std::move(pErrorHandle))
	, mAcceptors()
{
}

Listener::~Listener()
{
	SocketUtils::Close(mSocket);
}

HANDLE Listener::GetHandle() const
{
	return reinterpret_cast<HANDLE>(mSocket);  // NOLINT(performance-no-int-to-ptr)
}

void Listener::Dispatch(class IocpEvent& iocpEvent, uint32 numOfBytes)
{
	if (iocpEvent.GetEventType() != eIocpEventType::Accept)
	{
		NET_ASSERT(false, "Listener::Dispatch - eIocpEventType is not Accept");
		return;
	}

	const auto* pAcceptEvent = static_cast<IocpAcceptEvent*>(&iocpEvent);
	mAcceptors[pAcceptEvent->GetAcceptorIndex()]->Process();
}

SOCKET Listener::GetSocket() const
{
	return mSocket;
}

ListenerRef Listener::GetListenerRef()
{
	return std::static_pointer_cast<Listener>(shared_from_this());
}

bool Listener::StartAccept(const ServerServiceRef& pServerService)
{
	if (pServerService == nullptr)
	{
		NET_ENGINE_LOG_FATAL("pServerService is nullptr");
		CrashReporter::Crash();
	}

	if (SocketUtils::CreateTcpSocket(mSocket) == false)
	{
		NET_ENGINE_LOG_FATAL("SocketUtils::CreateTcpSocket is failed - errorCode : {}", WSAGetLastError());
		CrashReporter::Crash();
	}

	if (pServerService->GetIocpCore()->Register(shared_from_this()) == false)
	{
		NET_ENGINE_LOG_FATAL("pServerService->GetIocpCore()->Register is failed - errorCode : {}", WSAGetLastError());
		CrashReporter::Crash();
	}

	if (SocketUtils::SetReuseAddress(mSocket, true) == false)
	{
		NET_ENGINE_LOG_FATAL("SocketUtils::SetReuseAddress is failed - errorCode : {}", WSAGetLastError());
		CrashReporter::Crash();
	}

	if (SocketUtils::SetKeepAlive(mSocket, 30000, 1000) == false)
	{
		NET_ENGINE_LOG_FATAL("SocketUtils::SetKeepAlive - errorCode : {}", WSAGetLastError());
		CrashReporter::Crash();
	}

	if (SocketUtils::SetLinger(mSocket, 1, 0) == false)
	{
		NET_ENGINE_LOG_FATAL("SocketUtils::SetLinger is failed - errorCode : {}", WSAGetLastError());
		CrashReporter::Crash();
	}

	if (SocketUtils::Bind(mSocket, pServerService->GetNetAddress().GetSockAddr()) == false)
	{
		NET_ENGINE_LOG_FATAL("SocketUtils::Bind is failed - errorCode : {}", WSAGetLastError());
		CrashReporter::Crash();
	}

	if (SocketUtils::Listen(mSocket, SOMAXCONN_HINT(65535)) == false)
	{
		NET_ENGINE_LOG_FATAL("SocketUtils::Listen is failed - errorCode : {}", WSAGetLastError());
		CrashReporter::Crash();
	}
	
	mAcceptors.reserve(mAcceptCount);
	for (int32 i = 0; i < mAcceptCount; ++i)
	{
		const AcceptorRef pAcceptor = cpp_net_engine::MakeShared<Acceptor>(i);
		pAcceptor->SetOwner(GetListenerRef());
		pAcceptor->SetService(pServerService);
		mAcceptors.emplace_back(pAcceptor);
		pAcceptor->Register();
	}

	return true;
}

void Listener::CloseAccept()
{
	SocketUtils::Close(mSocket);
	for (const auto& pAcceptor : mAcceptors)
	{
		pAcceptor->Clear();
	}
}
