#include "LoginPacketHandler.h"

bool LoginPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
	const PacketSessionRef& pSession)
{
	return true;
}

bool LoginPacketHandler::HANDLE_S2C_LOGIN_RES(const Protocol::S2C_LOGIN_RES& packet, const PacketSessionRef& pSession)
{
	UE_LOG(LogTemp, Warning, TEXT("Success"));

	return true;
}
