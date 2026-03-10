#include "EchoPacketHandler.h"

bool EchoPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint32 packetId, byte* pBuffer,
	PacketSessionRef& pSession)
{
	return true;
}

bool EchoPacketHandler::HANDLE_S2C_ECHO_RES(const Protocol::S2C_ECHO_RES& packet, PacketSessionRef& pSession)
{
	return true;
}
