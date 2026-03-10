#include "pch.h"
#include "PacketHandler/EchoPacketHandler.h"

#include "Player.h"
#include "PlayerManager.h"
#include "Service.h"

bool EchoPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint32 packetId, byte* pBuffer,
                                                 PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("EchoPacketHandler::HANDLE_PACKET_ID_INVALID\n");

	return false;
}

bool EchoPacketHandler::HANDLE_S2C_ECHO_RES(const Protocol::S2C_ECHO_RES& packet, PacketSessionRef& pSession)
{
	Protocol::C2S_ECHO_REQ retPacket;

	retPacket.set_ehcomsg("Hello World\n");
	const auto pSendBuffer = EchoPacketHandler::MakeSendBuffer(retPacket);
	
	const PlayerRef pPlayer = PlayerManager::GetInstance().FindPlayer(pSession);
	if (pPlayer == nullptr)
	{
		return false;
	}

	pPlayer->PostDelay(3000, [pSession, pSendBuffer]()->void
		{
			pSession->Send(pSendBuffer);
		});

	return true;
}