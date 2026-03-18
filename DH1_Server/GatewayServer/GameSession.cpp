#include "pch.h"
#include "GameSession.h"

void GameSession::OnConnected()
{
}

void GameSession::OnEnterWaitQueue(const uint64 myTicket)
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
