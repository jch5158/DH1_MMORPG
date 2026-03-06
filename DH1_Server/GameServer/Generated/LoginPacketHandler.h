// ReSharper disable CppInconsistentNaming
#pragma once
#include "Cpp/PacketId.pb.h"
#include "Cpp/Enum.pb.h"
#include "Cpp/Struct.pb.h"
#include "Cpp/Login.pb.h"
#include "StlTypes.h"
#include "PacketSession.h"
#include <functional>

class LoginPacketHandler
{
public:

    using PacketHandle = std::function<bool(const uint16, byte*, PacketSessionRef&)>;

    static void Init()
    {
        
		sPacketHandleMap[Protocol::ePacketId::ID_C2S_LOGIN_REQ] = [](const uint16 size, byte* pBuffer, PacketSessionRef& pSession)->bool
			{
				return HandlePacket<Protocol::C2S_LOGIN_REQ>(size, pBuffer, pSession, HANDLE_C2S_LOGIN_REQ);
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
    static bool HANDLE_C2S_LOGIN_REQ(const Protocol::C2S_LOGIN_REQ& packet, PacketSessionRef& pSession);
    
    
    static NetSendBufferRef MakeSendBuffer(Protocol::S2C_LOGIN_RES& packet) { return MakeSendBuffer(packet, static_cast<uint32>(Protocol::ID_S2C_LOGIN_RES)); }
    

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
	static NetSendBufferRef MakeSendBuffer(T& packet, const uint32 packetId)
	{
		const uint16 dataSize = static_cast<uint16>(packet.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

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
			NET_ENGINE_LOG_FATAL("LoginPacketHandler::MakeSendBuffer SerializeToArray is Failed, &header[1] : {}, packetId : {}, dataSize : {}", fmt::ptr(&header[1]), header->id, dataSize);
			CrashReporter::Crash();
	    }
		
        sendBuffer->Commit(packetSize);
		
        return sendBuffer;
	}

	static HashMap<uint32, PacketHandle> sPacketHandleMap;
};