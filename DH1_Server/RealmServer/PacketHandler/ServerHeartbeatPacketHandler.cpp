#include "pch.h"
#include "ServerHeartbeatPacketHandler.h"
#include "WorldServerSession.h"

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
	const auto pWorldServerSession = std::static_pointer_cast<WorldServerSession>(pSession);
	if (pWorldServerSession == nullptr)
	{
		return false;
	}

	pWorldServerSession->UpdateHeartbeat(packet.serverid(), packet.sessioncount(), packet.status());

	NET_ENGINE_LOG_TRACE("S2S_HEARTBEAT_NOT - serverId: {}, sessionCount: {}, status: {}",
		packet.serverid(), packet.sessioncount(), packet.status());

	return true;
}
