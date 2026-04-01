#include "pch.h"
#include "ChatPacketHandler.h"

bool ChatPacketHandler::Validate(const PacketSessionRef& pSession)
{
	(void)pSession;
	return true;
}

bool ChatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("ChatPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool ChatPacketHandler::HANDLE_S2S_REALM_CHAT_SUBMIT_NOT(const Protocol::S2S_REALM_CHAT_SUBMIT_NOT& packet, const PacketSessionRef& pSession)
{
	(void)pSession;
	NET_ENGINE_LOG_INFO(
		"[CHAT][TEMP] Realm S2S_REALM_CHAT_SUBMIT_NOT sender_account_id={} msg_len={} server_timestamp_ms={}",
		packet.sender_account_id(),
		static_cast<int32>(packet.message().size()),
		packet.server_timestamp_ms());
	return true;
}
