#include "pch.h"
#include "Generated/EchoPacketHandler.h"

#include "Player.h"
#include "PlayerManager.h"
#include "Service.h"

bool EchoPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint32 packetId, byte* pBuffer,
                                                 PacketSessionRef& pSession)
{
	return false;
}

bool EchoPacketHandler::HANDLE_S2C_ECHO_RES(const Protocol::S2C_ECHO_RES& packet, PacketSessionRef& pSession)
{
	fmt::print("{}", packet.ehcomsg());

	Protocol::C2S_ECHO_REQ retPacket;
	retPacket.set_ehcomsg("Hello World\n");
	const auto pSendBuffer = EchoPacketHandler::MakeSendBuffer(retPacket);
	
	const PlayerRef pPlayer = PlayerManager::GetInstance().FindPlayer(pSession);
	if (pPlayer == nullptr)
	{
		return false;
	}

	pPlayer->PostDelay(pSession->GetService()->GetActorScheduler(), 3000, [pSession, pSendBuffer]()->void
		{
			pSession->Send(pSendBuffer);
		});

	return true;
}

HashMap<uint32, EchoPacketHandler::PacketHandle> EchoPacketHandler::sPacketHandleMap;