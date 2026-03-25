#include "pch.h"
#include "WorldServerSession.h"
#include "PacketHandler/PacketServiceTypeHandler.h"

WorldServerSession::WorldServerSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	: PacketSession(receiveBufferSize, maxPacketSize)
{
}

void WorldServerSession::OnConnected()
{
	NET_ENGINE_LOG_INFO("WorldServerSession::OnConnected - WorldServer connected, sessionId: {}", GetSessionId());
}

void WorldServerSession::OnDisconnecting(const eDisconnectReason reason)
{
	NET_ENGINE_LOG_INFO("WorldServerSession::OnDisconnecting - sessionId: {}, reason: {}", GetSessionId(), static_cast<int32>(reason));
}

void WorldServerSession::OnDisconnected()
{
	NET_ENGINE_LOG_INFO("WorldServerSession::OnDisconnected - WorldServer disconnected, sessionId: {}", GetSessionId());
}

void WorldServerSession::OnSend(const int32 len)
{
}

void WorldServerSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, GetPacketSessionRef());
}
