#include "pch.h"
#include "HeartbeatPacketHandler.h"
#include "ClientSession.h"

bool HeartbeatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
                                                      const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("HeartbeatPacketHandler::HANDLE_PACKET_ID_INVALID - packetId: {}", packetId);
	return false;
}

bool HeartbeatPacketHandler::HANDLE_C2S_HEARTBEAT_REQ(const Protocol::C2S_HEARTBEAT_REQ& packet, const PacketSessionRef& pSession)
{
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	if (!pClientSession->IsLoggedIn())
	{
		NET_ENGINE_LOG_WARN("HeartbeatPacketHandler - Heartbeat from unauthenticated session, disconnecting");
		return false;
	}

	Protocol::S2C_HEARTBEAT_RES retPacket;
	retPacket.set_clienttimestamp(packet.clienttimestamp());
	retPacket.set_servertimestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());

	pSession->Send(MakeSendBuffer(retPacket));
	return true;
}
