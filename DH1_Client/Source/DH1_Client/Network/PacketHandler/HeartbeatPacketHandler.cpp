#include "HeartbeatPacketHandler.h"

bool HeartbeatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
	const PacketSessionRef& pSession)
{
	return true;
}

bool HeartbeatPacketHandler::HANDLE_S2C_HEARTBEAT_RES(const Protocol::S2C_HEARTBEAT_RES& packet, const PacketSessionRef& pSession)
{
	// TODO: 서버 하트비트 응답 처리
	return true;
}
