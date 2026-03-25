#include "pch.h"
#include "ServerHeartbeatPacketHandler.h"
#include "GatewaySession.h"
#include "RealmSession.h"

bool ServerHeartbeatPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool ServerHeartbeatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("ServerHeartbeatPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool ServerHeartbeatPacketHandler::HANDLE_S2S_HEARTBEAT_NOT(const Protocol::S2S_HEARTBEAT_NOT& packet, const PacketSessionRef& pSession)
{
	const int32 serverType = packet.servertype();

	if (serverType == static_cast<int32>(Protocol::eRole::GATEWAY_SERVER))
	{
		const auto pGatewaySession = std::static_pointer_cast<GatewaySession>(pSession);
		if (pGatewaySession != nullptr)
		{
			pGatewaySession->UpdateHeartbeat(packet.serverid());
		}
	}
	else if (serverType == static_cast<int32>(Protocol::eRole::REALM_SERVER))
	{
		const auto pRealmSession = std::static_pointer_cast<RealmSession>(pSession);
		if (pRealmSession != nullptr)
		{
			pRealmSession->UpdateHeartbeat();
		}
	}

	NET_ENGINE_LOG_TRACE("S2S_HEARTBEAT_NOT - serverType: {}, serverId: {}, sessionCount: {}, status: {}",
		serverType, packet.serverid(), packet.sessioncount(), packet.status());

	return true;
}
