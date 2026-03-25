#include "pch.h"
#include "HeartbeatPacketHandler.h"
#include "ClientSession.h"

bool HeartbeatPacketHandler::Validate(const PacketSessionRef& pSession)
{
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	if (!pClientSession->IsLoggedIn())
	{
		return false;
	}

	return true;
}

bool HeartbeatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
                                                      const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("HeartbeatPacketHandler::HANDLE_PACKET_ID_INVALID - packetId: {}", packetId);
	return false;
}

bool HeartbeatPacketHandler::HANDLE_C2S_HEARTBEAT_NOT(const Protocol::C2S_HEARTBEAT_NOT& packet, const PacketSessionRef& pSession)
{
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	if (pClientSession == nullptr)
	{
		return false;
	}

	pClientSession->UpdateHeartbeat();

	NET_ENGINE_LOG_TRACE("C2S_HEARTBEAT_NOT - sessionId: {}, accountId: {}", pSession->GetSessionId(), pClientSession->GetAccountId());
	return true;
}
