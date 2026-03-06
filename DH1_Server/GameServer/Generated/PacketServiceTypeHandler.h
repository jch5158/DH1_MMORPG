#pragma once
#include "LoginPacketHandler.h"


class PacketServiceTypeHandler
{
public:

	using PacketServiceTypeHandle = std::function<bool(const uint16, const uint32, byte*, PacketSessionRef&)>;

	static constexpr uint16 GET_SERVICE_TYPE(const uint32 packetId) { return (packetId >> 16) & 0x0000FFFF; }
	static constexpr uint16 GET_PACKET_ID(const uint32 packetId) { return packetId & 0x0000FFFF; }

	static void Init()
	{
        LoginPacketHandler::Init();

		
        sPacketServiceTypeMap[Protocol::eServiceType::SERVICE_TYPE_LOGIN] = [](const uint16 size, const uint32 packetId, byte* pBuffer, PacketSessionRef& pSession) -> bool
			{
				return LoginPacketHandler::HandlePacket(size, packetId, pBuffer, pSession);
			};

	}

	static bool HandlePacketServiceType(const uint16 len, byte* pBuffer, PacketSessionRef& pSession)
	{
		const auto [packetSize, id] = *(reinterpret_cast<PacketHeader*>(pBuffer));
		
		const uint16 serviceType = GET_SERVICE_TYPE(id);
		const uint16 packetId = GET_PACKET_ID(id);

		const auto iter = sPacketServiceTypeMap.find(serviceType);
		if (iter != sPacketServiceTypeMap.end())
		{
			return iter->second(packetSize, id, pBuffer, pSession);
		}

		return HANDLE_SERVICE_TYPE_INVALID(packetSize, id, pBuffer, pSession);
	}

	static bool HANDLE_SERVICE_TYPE_INVALID(const uint16 size, const uint16 packetId, byte* pBuffer, PacketSessionRef& pSession);

private:

	static HashMap<uint32, PacketServiceTypeHandle> sPacketServiceTypeMap;
};