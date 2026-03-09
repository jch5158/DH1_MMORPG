#pragma once

class SocketUtils final  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:

	SocketUtils() = delete;
	~SocketUtils() = delete;
	SocketUtils(const SocketUtils&) = delete;
	SocketUtils& operator=(const SocketUtils&) = delete;
	SocketUtils(SocketUtils&&) = delete;
	SocketUtils& operator=(SocketUtils&&) = delete;

	static bool Init();
	static void Clear();
	static bool CreateTcpSocket(SOCKET& outSocket);
	static void Close(SOCKET& socket);
	static bool SetLinger(const SOCKET socket, const uint16 onOff, const uint16 linger);
	static bool SetReuseAddress(const SOCKET socket, bool flag);
	static bool SetKeepAlive(const SOCKET socket, const uint32 timeMs, const uint32 intervalMs);
	static bool SetRecvBufferSize(const SOCKET socket, int32 size);
	static bool SetSendBufferSize(const SOCKET socket, int32 size);
	static bool SetTcpNoDelay(const SOCKET socket, bool flag);
	static bool SetUpdateAcceptSocket(const SOCKET socket, const SOCKET listenSocket);
	static bool Bind(const SOCKET socket, const SOCKADDR_IN& sockAddr);
	static bool BindAnyAddress(const SOCKET socket, const uint16 port);
	static bool Listen(const SOCKET socket, const int32 backlog);
	static bool AcceptEx(const SOCKET listenSock, const SOCKET acceptSock, void* pOutputBuffer, OVERLAPPED* pOverlapped);
	static bool ConnectEx(const SOCKET socket, SOCKADDR_IN& sockAddr, OVERLAPPED* pOverlapped);
	static bool DisconnectEx(const SOCKET socket, OVERLAPPED* pOverlapped);
	static void CancelIoEx(const SOCKET socket, OVERLAPPED* pOverlapped);
	static bool WsaSend(const SOCKET socket, WSABUF* pWsabuf, const int32 bufSize, OVERLAPPED* pOverlapped);
	static bool WsaReceive(const SOCKET socket, WSABUF* pWsabuf, const int32 bufSize, OVERLAPPED* pOverlapped);

private:

	static bool bindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);

	static LPFN_ACCEPTEX acceptEx;
	static LPFN_CONNECTEX connectEx;
	static LPFN_DISCONNECTEX disconnectEx;
};

template<typename T>
static inline bool SetSockOpt(const SOCKET socket, const int32 level, const int32 optName, T optVal)
{
	return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char*>(&optVal), sizeof(T));
}

