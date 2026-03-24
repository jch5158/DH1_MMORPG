#include "pch.h"
#include "GameSession.h"
#include "PacketHandler/PacketServiceTypeHandler.h"

GameSession::GameSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	:PacketSession(receiveBufferSize, maxPacketSize)
{
}

void GameSession::OnConnected()
{
}

void GameSession::OnDisconnecting(const eDisconnectReason reason)
{
	if (reason != eDisconnectReason::Closed && reason != eDisconnectReason::Timeout)
	{
		NET_ENGINE_LOG_ERROR("[Server GameSession] Unexpected disconnect! sessionId: {}, reason: {}", GetSessionId(), static_cast<uint8>(reason));
	}
}

void GameSession::OnDisconnected()
{
}

void GameSession::OnSend(const int32 len)
{
}

void GameSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketSessionRef pSession = GetPacketSessionRef();

	if (PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, pSession) == false)
	{
		pSession->Disconnect(eDisconnectReason::Kicked);
	}
}

void GameSession::OnError(const int32 errorCode)
{
	NET_ENGINE_LOG_ERROR("[Server GameSession] OnError - sessionId: {}, errorCode: {}", GetSessionId(), errorCode);
}
