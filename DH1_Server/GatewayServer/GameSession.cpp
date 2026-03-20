#include "pch.h"
#include "GameSession.h"

GameSession::GameSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	:PacketSession(receiveBufferSize, maxPacketSize)
{
}

void GameSession::OnConnected()
{
}

void GameSession::OnDisconnecting(const eDisconnectReason reason)
{
}

void GameSession::OnDisconnected()
{
}

void GameSession::OnSend(const int32 len)
{
}

void GameSession::OnReceivePacket(byte* pBuffer, const int32 len)
{
}

void GameSession::OnError(const int32 errorCode)
{
}
