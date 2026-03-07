namespace PacketGenerator
{
    internal static class PacketFormatter
    {
        public static readonly string HANDLE_SERVICE_TYPE_FILE_FORMAT =
            @"#pragma once
{0}

class PacketServiceTypeHandler
{{
public:

	using PacketServiceTypeHandle = std::function<bool(const uint16, const uint32, byte*, PacketSessionRef&)>;

	static constexpr uint16 GET_SERVICE_TYPE(const uint32 packetId) {{ return (packetId >> 16) & 0x0000FFFF; }}
	static constexpr uint16 GET_PACKET_ID(const uint32 packetId) {{ return packetId & 0x0000FFFF; }}

	static void Init()
	{{
        {1}
		{2}
	}}

	static bool HandlePacketServiceType(const uint16 len, byte* pBuffer, PacketSessionRef& pSession)
	{{
		const auto [packetSize, id] = *(reinterpret_cast<PacketHeader*>(pBuffer));
		
		const uint16 serviceType = GET_SERVICE_TYPE(id);

		const auto iter = sPacketServiceTypeMap.find(serviceType);
		if (iter != sPacketServiceTypeMap.end())
		{{
			return iter->second(packetSize, id, pBuffer, pSession);
		}}

		return HANDLE_SERVICE_TYPE_INVALID(packetSize, id, pBuffer, pSession);
	}}

	static bool HANDLE_SERVICE_TYPE_INVALID(const uint16 size, const uint16 packetId, byte* pBuffer, PacketSessionRef& pSession);

private:

	static HashMap<uint32, PacketServiceTypeHandle> sPacketServiceTypeMap;
}};";

        public static readonly string SERVICE_TYPE_INCLUDE_FORMAT =
            @"#include ""{0}PacketHandler.h""
";

        public static readonly string SERVICE_TYPE_INIT_FORMAT =
            @"
        {0}PacketHandler::Init();
";

        public static readonly string SERVICE_TYPE_HANDLE_INIT_FORMAT =
            @"
        sPacketServiceTypeMap[Protocol::eServiceType::{0}] = [](const uint16 size, const uint32 packetId, byte* pBuffer, PacketSessionRef& pSession) -> bool
			{{
				return {1}::HandlePacket(size, packetId, pBuffer, pSession);
			}};
";

        public static readonly string HANDLE_FILE_FORMAT =
            @"// ReSharper disable CppInconsistentNaming
#pragma once
#include ""Cpp/PacketId.pb.h""
#include ""Cpp/Enum.pb.h""
#include ""Cpp/Struct.pb.h""
#include ""Cpp/{0}.pb.h""
#include ""StlTypes.h""
#include ""PacketSession.h""
#include <functional>

class {0}PacketHandler
{{
public:

    using PacketHandle = std::function<bool(const uint16, byte*, PacketSessionRef&)>;

    static void Init()
    {{
        {1}
    }}

	static bool HandlePacket(const uint16 size, const uint32 packetId, byte* pBuffer, PacketSessionRef& pSession)
	{{
		const auto iter = sPacketHandleMap.find(packetId);
		if (iter != sPacketHandleMap.end())
		{{
			return iter->second(size, pBuffer, pSession);
		}}

		return HANDLE_PACKET_ID_INVALID(size, packetId, pBuffer, pSession);
	}}

	static bool HANDLE_PACKET_ID_INVALID(const uint16 size, const uint32 packetId, byte* pBuffer, PacketSessionRef& pSession);
    {2}
    
    {3}

private:

	template<typename PacketType, typename Handle>
	static bool HandlePacket(const uint16 size, byte* pBuffer, PacketSessionRef& pSession, Handle handlePacket)
	{{
		PacketType packet{{}};
		if (packet.ParseFromArray(pBuffer + SIZE_OF_16(PacketHeader), size - SIZE_OF_16(PacketHeader)) == false)
		{{
			return false;
		}}

		return handlePacket(packet, pSession);
	}}

    template<typename T>
	static NetSendBufferRef MakeSendBuffer(const T& packet, const uint32 packetId)
	{{
		const uint16 dataSize = static_cast<uint16>(packet.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

	    if (packetSize > std::numeric_limits<uint16>::max())
	    {{
		    NET_ENGINE_LOG_FATAL(""MakeSendBuffer Size Overflow, packetId : {{}}, packetSize : {{}}"", packetId, packetSize);
		    CrashReporter::Crash();
	    }}

		auto sendBuffer = cpp_net_engine::MakeSendBuffer(packetSize);

		byte* pBuffer = sendBuffer->Reserve(packetSize);
		if (pBuffer == nullptr)
		{{
			NET_ENGINE_LOG_FATAL(""MakeSendBuffer sendBuffer->Reserve(packetSize) is failed"");
			CrashReporter::Crash();
		}}

		auto* header = reinterpret_cast<PacketHeader*>(pBuffer);
		header->size = packetSize;
		header->id = packetId;
		if (!packet.SerializeToArray(&header[1], dataSize))
		{{
			NET_ENGINE_LOG_FATAL(""{0}PacketHandler::MakeSendBuffer SerializeToArray is Failed, &header[1] : {{}}, packetId : {{}}, dataSize : {{}}"", fmt::ptr(&header[1]), header->id, dataSize);
			CrashReporter::Crash();
	    }}
		
        sendBuffer->Commit(packetSize);
		
        return sendBuffer;
	}}

	static HashMap<uint32, PacketHandle> sPacketHandleMap;
}};";

        public static readonly string HANDLE_INIT_FORMAT =
@"
		sPacketHandleMap[Protocol::ePacketId::{0}] = [](const uint16 size, byte* pBuffer, PacketSessionRef& pSession)->bool
			{{
				return HandlePacket<Protocol::{1}>(size, pBuffer, pSession, HANDLE_{1});
			}};
		";


		public static readonly string HANDLE_DECLARE_FORMAT =
            @"static bool HANDLE_{0}(const Protocol::{0}& packet, PacketSessionRef& pSession);
    ";

        public static readonly string MAKE_SEND_BUFFER_FORMAT =
            @"static NetSendBufferRef MakeSendBuffer(Protocol::{0}& packet) {{ return MakeSendBuffer(packet, static_cast<uint32>(Protocol::{1})); }}
    ";
    }
}
