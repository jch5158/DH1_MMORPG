#include "pch.h"
#include "PacketHandler/LoginPacketHandler.h"


bool LoginPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint32 packetId, byte* pBuffer,
	PacketSessionRef& pSession)
{
	return false;
}

bool LoginPacketHandler::HANDLE_C2S_LOGIN_REQ(const Protocol::C2S_LOGIN_REQ& packet, PacketSessionRef& pSession)
{
	return false;
}
