#include "Network/CppNetEngine/NetSession.h"

NetSession::NetSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	:PacketSession(receiveBufferSize, maxPacketSize)
{}

NetSession::~NetSession()
{
}

void NetSession::OnConnected()
{
}

void NetSession::OnDisconnecting(const eDisconnectReason reason)
{
}

void NetSession::OnDisconnected()
{
}

void NetSession::OnSend(const int32 len)
{
}

void NetSession::OnError(const int32 errorCode)
{
}

void NetSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
}
