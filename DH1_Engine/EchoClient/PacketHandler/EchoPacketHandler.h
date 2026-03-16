// ReSharper disable CppInconsistentNaming
#pragma once

#ifdef _UNREAL_

#include "Network/CppNetEngine/NetEngineWrapper.h"
#include "Network/ProtocolHeader/ProtocolWrapper.h"

#else

#include "PacketId.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"
#include "Echo.pb.h"
#include "StlTypes.h"
#include "PacketSession.h"

#endif

class EchoPacketHandler
{
public:

    using PacketHandle = std::function<bool(const uint16, byte*, PacketSessionRef&)>;

    static void Init()
    {
        sPacketHandleMap[Protocol::ePacketId::ID_S2C_ECHO_RES] = [](const uint16 size, byte* pBuffer, PacketSessionRef& pSession)->bool
			{
				return HandlePacket<Protocol::S2C_ECHO_RES>(size, pBuffer, pSession, HANDLE_S2C_ECHO_RES);
			};

    }

	static bool HandlePacket(const uint16 size, const uint32 packetId, byte* pBuffer, PacketSessionRef& pSession)
	{
		const auto iter = sPacketHandleMap.find(packetId);
		if (iter != sPacketHandleMap.end())
		{
			return iter->second(size, pBuffer, pSession);
		}

		return HANDLE_PACKET_ID_INVALID(size, packetId, pBuffer, pSession);
	}

	static bool HANDLE_PACKET_ID_INVALID(const uint16 size, const uint32 packetId, byte* pBuffer, PacketSessionRef& pSession);
    static bool HANDLE_S2C_ECHO_RES(const Protocol::S2C_ECHO_RES& packet, PacketSessionRef& pSession);

    
    static NetSendBufferRef MakeSendBuffer(Protocol::C2S_ECHO_REQ& packet) { return MakeSendBuffer(packet, static_cast<uint32>(Protocol::ID_C2S_ECHO_REQ)); }


private:

	template<typename PacketType, typename Handle>
	static bool HandlePacket(const uint16 size, byte* pBuffer, PacketSessionRef& pSession, Handle handlePacket)
	{
		PacketType packet{};
		if (packet.ParseFromArray(pBuffer + SIZE_OF_16(PacketHeader), size - SIZE_OF_16(PacketHeader)) == false)
		{
			return false;
		}

		return handlePacket(packet, pSession);
	}

    template<typename T>
	static NetSendBufferRef MakeSendBuffer(const T& packet, const uint32 packetId)
	{
		const uint16 dataSize = static_cast<uint16>(packet.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

	    if (packetSize > std::numeric_limits<uint16>::max())
	    {
		    NET_ENGINE_LOG_FATAL("MakeSendBuffer Size Overflow, packetId : {}, packetSize : {}", packetId, packetSize);
		    CrashReporter::Crash();
	    }

		auto sendBuffer = cpp_net_engine::MakeSendBuffer(packetSize);

		byte* pBuffer = sendBuffer->Reserve(packetSize);
		if (pBuffer == nullptr)
		{
			NET_ENGINE_LOG_FATAL("MakeSendBuffer sendBuffer->Reserve(packetSize) is failed");
			CrashReporter::Crash();
		}

		auto* header = reinterpret_cast<PacketHeader*>(pBuffer);
		header->size = packetSize;
		header->id = packetId;
		if (!packet.SerializeToArray(&header[1], dataSize))
		{
			NET_ENGINE_LOG_FATAL("EchoPacketHandler::MakeSendBuffer SerializeToArray is Failed, &header[1] : {}, packetId : {}, dataSize : {}", fmt::ptr(&header[1]), header->id, dataSize);
			CrashReporter::Crash();
	    }
		
        sendBuffer->Commit(packetSize);
		
        return sendBuffer;
	}

	inline static HashMap<uint32, PacketHandle> sPacketHandleMap;
};