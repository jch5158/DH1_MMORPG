#include "pch.h"
#include "PacketHandler/PacketServiceTypeHandler.h"

bool PacketServiceTypeHandler::HANDLE_SERVICE_TYPE_INVALID(const uint16 size, const uint32 packetId, byte* pBuffer,
	PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("PacketServiceTypeHandler::HANDLE_SERVICE_TYPE_INVALID\n");

	return false;
}
