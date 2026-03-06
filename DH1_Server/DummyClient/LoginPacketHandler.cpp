#include "pch.h"
#include "Generated/LoginPacketHandler.h"

bool LoginPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint32 packetId, byte* pBuffer, PacketSessionRef& pSession)
{
	return true;
}

bool LoginPacketHandler::HANDLE_S2C_LOGIN_RES(const Protocol::S2C_LOGIN_RES& packet, PacketSessionRef& pSession)
{
	return true;
}

HashMap<uint32, LoginPacketHandler::PacketHandle> LoginPacketHandler::sPacketHandleMap;
