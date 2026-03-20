#include "pch.h"
#include "PacketServiceTypeHandler.h"

bool PacketServiceTypeHandler::HANDLE_SERVICE_TYPE_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
	PacketSessionRef& pSession)
{
	return false;
}
