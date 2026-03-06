#include "pch.h"
#include "Generated/PacketServiceTypeHandler.h"

bool PacketServiceTypeHandler::HANDLE_SERVICE_TYPE_INVALID(const uint16 size, const uint16 packetId, byte* pBuffer, PacketSessionRef& pSession)
{
	return false;
}

HashMap<uint32, PacketServiceTypeHandler::PacketServiceTypeHandle> PacketServiceTypeHandler::sPacketServiceTypeMap;
