#include "HeartbeatPacketHandler.h"

bool HeartbeatPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool HeartbeatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
	const PacketSessionRef& pSession)
{
	return true;
}
