#include "pch.h"
#include "PacketHandler/EchoPacketHandler.h"
#include "GameSession.h"

bool EchoPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("EchoPacketHandler::HANDLE_PACKET_ID_INVALID\n");
	return false;
}

bool EchoPacketHandler::HANDLE_C2S_ECHO_REQ(const Protocol::C2S_ECHO_REQ& packet, const PacketSessionRef& pSession)
{
	Protocol::S2C_ECHO_RES retPacket;
	retPacket.set_echomsg(packet.echomsg());

	const auto pSendBuffer = EchoPacketHandler::MakeSendBuffer(retPacket);

	pSession->Send(pSendBuffer);

	return true;
}
