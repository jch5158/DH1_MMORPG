#include "pch.h"
#include "GatewaySession.h"

GatewaySession::GatewaySession(const int32 receiveBufferSize, const int32 maxPacketSize)
	: PacketSession(receiveBufferSize, maxPacketSize)
{
}

void GatewaySession::OnConnected()
{
	NET_ENGINE_LOG_INFO("GatewaySession::OnConnected - Connected to GatewayServer");
}

void GatewaySession::OnDisconnecting(const eDisconnectReason reason)
{
	NET_ENGINE_LOG_INFO("GatewaySession::OnDisconnecting - reason: {}", static_cast<int32>(reason));
}

void GatewaySession::OnDisconnected()
{
	NET_ENGINE_LOG_INFO("GatewaySession::OnDisconnected - Disconnected from GatewayServer");
}

void GatewaySession::OnSend(const int32 len)
{
}

void GatewaySession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	// TODO: Handle packets from GatewayServer
}
