#include "pch.h"
#include "ServerHeartbeatPacketHandler.h"

bool ServerHeartbeatPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool ServerHeartbeatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("ServerHeartbeatPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool ServerHeartbeatPacketHandler::HANDLE_S2S_HEARTBEAT_REQ(const Protocol::S2S_HEARTBEAT_REQ& packet, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_TRACE("S2S_HEARTBEAT_REQ - serverId: {}, sessionCount: {}, status: {}",
		packet.serverid(), packet.sessioncount(), packet.status());

	Protocol::S2S_HEARTBEAT_RES response;
	response.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
	response.set_accepted(true);

	pSession->Send(ServerHeartbeatPacketHandler::MakeSendBuffer(response));

	return true;
}
