#include "pch.h"
#include "Generated/EchoPacketHandler.h"
#include "GameSession.h"

bool EchoPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint32 packetId, byte* pBuffer, PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("EchoPacketHandler::HANDLE_PACKET_ID_INVALID\n");
	return false;
}

bool EchoPacketHandler::HANDLE_C2S_ECHO_REQ(const Protocol::C2S_ECHO_REQ& packet, PacketSessionRef& pSession)
{
	//fmt::print("{}\n", packet.ehcomsg());

	Protocol::S2C_ECHO_RES retPacket;
	retPacket.set_ehcomsg("Hello World\n");
	const auto pSendBuffer = EchoPacketHandler::MakeSendBuffer(retPacket);
	pSession->Send(pSendBuffer);

	return true;
}

HashMap<uint32, EchoPacketHandler::PacketHandle> EchoPacketHandler::sPacketHandleMap;