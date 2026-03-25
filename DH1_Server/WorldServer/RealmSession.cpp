#include "pch.h"
#include "RealmSession.h"
#include "WorldService.h"
#include "PacketHandler/ServerHeartbeatPacketHandler.h"

RealmSession::RealmSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	: PacketSession(receiveBufferSize, maxPacketSize)
{
}

void RealmSession::OnConnected()
{
	NET_ENGINE_LOG_INFO("RealmSession::OnConnected - Connected to RealmServer, sessionId: {}", GetSessionId());

	const auto& worldService = WorldService::GetInstance();

	Protocol::S2S_HEARTBEAT_NOT packet;
	packet.set_servertype(static_cast<int32>(Protocol::eRole::WORLD_SERVER));
	packet.set_serverid(worldService.GetWorldServerId());
	packet.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
	packet.set_sessioncount(0);
	packet.set_status(static_cast<int32>(eWorldServerStatus::Online));

	Send(ServerHeartbeatPacketHandler::MakeSendBuffer(packet));
}

void RealmSession::OnDisconnecting(const eDisconnectReason reason)
{
	NET_ENGINE_LOG_INFO("RealmSession::OnDisconnecting - sessionId: {}, reason: {}", GetSessionId(), static_cast<int32>(reason));
}

void RealmSession::OnDisconnected()
{
	NET_ENGINE_LOG_INFO("RealmSession::OnDisconnected - Disconnected from RealmServer, sessionId: {}", GetSessionId());
}

void RealmSession::OnSend(const int32 len)
{
}

void RealmSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	// TODO: PacketServiceTypeHandler
}
