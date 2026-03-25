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
	NET_ENGINE_LOG_WARN("GatewaySession::OnDisconnected - GatewayServer disconnected, sessionId: {}, gatewayServerId: {}", GetSessionId(), mGatewayServerId);
}

void GatewaySession::OnSend(const int32 len)
{
}

void GatewaySession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, GetPacketSessionRef());
}

void GatewaySession::UpdateHeartbeat(const int32 serverId)
{
	mGatewayServerId = serverId;
	mLastHeartbeatMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count(), std::memory_order_release);
}

int64 GatewaySession::GetLastHeartbeatMs() const
{
	return mLastHeartbeatMs.load(std::memory_order_acquire);
}

int32 GatewaySession::GetGatewayServerId() const
{
	return mGatewayServerId;
}
