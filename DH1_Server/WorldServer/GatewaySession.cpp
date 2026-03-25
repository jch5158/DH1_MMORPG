#include "pch.h"
#include "GatewaySession.h"
#include "PacketHandler/PacketServiceTypeHandler.h"

GatewaySession::GatewaySession(const int32 receiveBufferSize, const int32 maxPacketSize)
	: PacketSession(receiveBufferSize, maxPacketSize)
{
}

void GatewaySession::OnConnected()
{
	NET_ENGINE_LOG_INFO("GatewaySession::OnConnected - GatewayServer connected, sessionId: {}", GetSessionId());
}

void GatewaySession::OnDisconnecting(const eDisconnectReason reason)
{
	NET_ENGINE_LOG_INFO("GatewaySession::OnDisconnecting - sessionId: {}, reason: {}", GetSessionId(), static_cast<int32>(reason));
}

void GatewaySession::OnDisconnected()
{
	NET_ENGINE_LOG_INFO("GatewaySession::OnDisconnected - GatewayServer disconnected, sessionId: {}", GetSessionId());
}

void GatewaySession::OnSend(const int32 len)
{
}

void GatewaySession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, GetPacketSessionRef());
}
