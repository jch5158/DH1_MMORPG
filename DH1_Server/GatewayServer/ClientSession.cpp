#include "pch.h"
#include "ClientSession.h"

ClientSession::ClientSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	:PacketSession(receiveBufferSize, maxPacketSize)
{
}

void ClientSession::OnConnected()
{
}

void ClientSession::OnDisconnecting(const eDisconnectReason reason)
{
}

void ClientSession::OnDisconnected()
{
}

void ClientSession::OnSend(const int32 len)
{
}

void ClientSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
}

void ClientSession::OnError(const int32 errorCode)
{
}
