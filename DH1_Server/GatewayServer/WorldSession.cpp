#include "pch.h"
#include "WorldSession.h"
#include "GatewayService.h"
#include "PacketHandler/PacketServiceTypeHandler.h"
#include "PacketHandler/ServerHeartbeatPacketHandler.h"

WorldSession::WorldSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	: PacketSession(receiveBufferSize, maxPacketSize)
{
}

void WorldSession::OnConnected()
{
	NET_ENGINE_LOG_INFO("WorldSession::OnConnected - Connected to WorldServer, sessionId: {}", GetSessionId());

	const auto& gatewayService = GatewayService::GetInstance();

	Protocol::S2S_HEARTBEAT_NOT packet;
	packet.set_servertype(static_cast<int32>(Protocol::eRole::GATEWAY_SERVER));
	packet.set_serverid(gatewayService.GetGatewayId());
	packet.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
	packet.set_sessioncount(0);
	packet.set_status(static_cast<int32>(eGatewayStatus::Online));

	Send(ServerHeartbeatPacketHandler::MakeSendBuffer(packet));
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

void WorldSession::UpdateHeartbeat(const int32 serverId)
{
	mWorldServerId = serverId;
	mLastHeartbeatMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count(), std::memory_order_release);
}

int64 WorldSession::GetLastHeartbeatMs() const
{
	return mLastHeartbeatMs.load(std::memory_order_acquire);
}

int32 WorldSession::GetWorldServerId() const
{
	return mWorldServerId;
}
