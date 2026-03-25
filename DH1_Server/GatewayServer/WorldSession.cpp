#include "pch.h"
#include "WorldSession.h"
#include "PacketHandler/PacketServiceTypeHandler.h"

WorldSession::WorldSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	: PacketSession(receiveBufferSize, maxPacketSize)
{
}

void WorldSession::OnConnected()
{
	NET_ENGINE_LOG_INFO("WorldSession::OnConnected - Connected to WorldServer, sessionId: {}", GetSessionId());
}

void WorldSession::OnDisconnecting(const eDisconnectReason reason)
{
	NET_ENGINE_LOG_INFO("WorldSession::OnDisconnecting - sessionId: {}, reason: {}", GetSessionId(), static_cast<int32>(reason));
}

void WorldSession::OnDisconnected()
{
	NET_ENGINE_LOG_INFO("WorldSession::OnDisconnected - Disconnected from WorldServer, sessionId: {}", GetSessionId());
}

void WorldSession::OnSend(const int32 len)
{
}

void WorldSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, GetPacketSessionRef());
}
