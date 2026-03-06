#include "pch.h"
#include "SocketUtils.h"

bool SocketUtils::Init()
{
	static bool isInit = false;
	if (isInit)
	{
		return true;
	}

	WSADATA wsaData{};
	if (WSAStartup(WINSOCK_VERSION, &wsaData) == SOCKET_ERROR)
	{
		return false;
	}

	SOCKET dummySocket;
	/* 런타임에 주소 얻어오는 API */
	if (CreateTcpSocket(dummySocket) == false)
	{
		return false;
	}

	if (bindWindowsFunction(dummySocket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&connectEx)) == false)
	{
		return false;
	}

	if (bindWindowsFunction(dummySocket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&disconnectEx)) == false)
	{
		return false;
	}

	if (bindWindowsFunction(dummySocket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&acceptEx)) == false)
	{
		return false;
	}
	
	Close(dummySocket);

	isInit = true;

	return true;
}

void SocketUtils::Clear()
{
	WSACleanup();
}

bool SocketUtils::CreateTcpSocket(SOCKET& outSocket)
{
	outSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == outSocket)
	{
		return false;
	}

	return true;
}

void SocketUtils::Close(SOCKET& socket)
{
	if (INVALID_SOCKET == socket)
	{
		closesocket(socket);
	}

	socket = INVALID_SOCKET;
}

bool SocketUtils::SetLinger(const SOCKET socket, const uint16 onOff, const uint16 linger)
{
	LINGER option{};
	option.l_onoff = onOff;
	option.l_linger = linger;
	return SetSockOpt(socket, SOL_SOCKET, SO_LINGER, option);
}

bool SocketUtils::SetReuseAddress(const SOCKET socket, const bool flag)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_REUSEADDR, flag);
}

bool SocketUtils::SetKeepAlive(const SOCKET socket, const uint32 timeMs, const uint32 intervalMs)
{
	tcp_keepalive keepAliveOpts;
	keepAliveOpts.onoff = 1;                    // 1: Keep-Alive 켜기, 0: 끄기
	keepAliveOpts.keepalivetime = timeMs;       // 유휴 대기 시간 (예: 30000 = 30초)
	keepAliveOpts.keepaliveinterval = intervalMs; // 응답 없을 시 재전송 간격 (예: 1000 = 1초)

	DWORD bytesReturned = 0;

	const int result = WSAIoctl(
		socket,
		SIO_KEEPALIVE_VALS,
		&keepAliveOpts, sizeof(keepAliveOpts),
		nullptr, 0,
		&bytesReturned,
		nullptr, nullptr
	);

	return result != SOCKET_ERROR;
}

bool SocketUtils::SetRecvBufferSize(const SOCKET socket, const int32 size)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_RCVBUF, size);
}

bool SocketUtils::SetSendBufferSize(const SOCKET socket, const int32 size)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_SNDBUF, size);
}

bool SocketUtils::SetTcpNoDelay(const SOCKET socket, const bool flag)
{
	return SetSockOpt(socket, SOL_SOCKET, TCP_NODELAY, flag);
}

// ListenSocket의 특성을 ClientSocket에 그대로 적용
bool SocketUtils::SetUpdateAcceptSocket(const SOCKET socket, const SOCKET listenSocket)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
}

bool SocketUtils::Bind(const SOCKET socket, const SOCKADDR_IN& sockAddr)
{
	return SOCKET_ERROR != bind(socket, reinterpret_cast<const SOCKADDR*>(&sockAddr), sizeof(SOCKADDR_IN));
}

bool SocketUtils::BindAnyAddress(const SOCKET socket, const uint16 port)
{
	SOCKADDR_IN myAddress{};
	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr = ::htonl(INADDR_ANY);
	myAddress.sin_port = ::htons(port);

	return SOCKET_ERROR != bind(socket, reinterpret_cast<const SOCKADDR*>(&myAddress), sizeof(myAddress));
}

bool SocketUtils::Listen(const SOCKET socket, const int32 backlog)
{
	return SOCKET_ERROR != ::listen(socket, backlog);
}

bool SocketUtils::AcceptEx(const SOCKET listenSock, const SOCKET acceptSock, void* pOutputBuffer, OVERLAPPED* pOverlapped)
{
	DWORD byteReceived = 0;
	const int32 retVal = acceptEx(listenSock, acceptSock, pOutputBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &byteReceived, pOverlapped);
	return retVal == TRUE;
}

bool SocketUtils::ConnectEx(const SOCKET socket, SOCKADDR_IN& sockAddr, OVERLAPPED* pOverlapped)
{
	DWORD numOfBytes = 0;
	return connectEx(socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(SOCKADDR_IN), nullptr, 0, &numOfBytes, pOverlapped);
}

bool SocketUtils::DisconnectEx(const SOCKET socket, OVERLAPPED* pOverlapped)
{
	return disconnectEx(socket, pOverlapped, TF_REUSE_SOCKET, 0);
}

void SocketUtils::CancelIoEx(const SOCKET socket, OVERLAPPED* pOverlapped)
{
	::CancelIoEx(reinterpret_cast<HANDLE>(socket), pOverlapped);  // NOLINT(performance-no-int-to-ptr)
}

bool SocketUtils::WsaSend(const SOCKET socket, WSABUF* pWsabuf, const int32 bufSize, OVERLAPPED* pOverlapped)
{
	DWORD numOfBytes = 0;
	return SOCKET_ERROR != WSASend(socket, pWsabuf, static_cast<DWORD>(bufSize), &numOfBytes, 0, pOverlapped, nullptr);
}

bool SocketUtils::WsaReceive(const SOCKET socket, WSABUF* pWsabuf, const int32 bufSize, OVERLAPPED* pOverlapped)
{
	DWORD numOfBytes = 0;
	DWORD flags = 0;
	return SOCKET_ERROR != WSARecv(socket, pWsabuf, bufSize, &numOfBytes, &flags, pOverlapped, nullptr);
}

bool SocketUtils::bindWindowsFunction(const SOCKET socket, GUID guid, LPVOID* fn)
{
	DWORD bytes = 0;
	return SOCKET_ERROR != WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), static_cast<LPVOID*>(fn), sizeof(*fn), OUT & bytes, nullptr, nullptr);
}

LPFN_ACCEPTEX SocketUtils::acceptEx = nullptr;
LPFN_CONNECTEX SocketUtils::connectEx = nullptr;
LPFN_DISCONNECTEX SocketUtils::disconnectEx = nullptr;
