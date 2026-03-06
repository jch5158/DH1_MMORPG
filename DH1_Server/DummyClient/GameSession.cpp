#include "pch.h"
#include "Generated/PacketServiceTypeHandler.h"
#include "GameSession.h"

#include "Player.h"
#include "PlayerManager.h"

GameSession::GameSession()
	:PacketSession()
{
}

GameSession::~GameSession()
{
}

void GameSession::OnConnected()
{
	const PacketSessionRef pSession = std::static_pointer_cast<PacketSession>(shared_from_this());
	const PlayerRef pPlayer = cpp_net_engine::MakeShared<Player>();
	
	if (pPlayer->SetSession(pSession) == false)
	{
		return;
	}

	if (PlayerManager::GetInstance().AddPlayer(pSession, pPlayer) == false)
	{
		pPlayer->ResetSession();
		return;
	}

	Protocol::C2S_ECHO_REQ packet;
	packet.set_ehcomsg("Hello World\n");
	const auto pSendBuffer = EchoPacketHandler::MakeSendBuffer(packet);
	Send(pSendBuffer);
}

void GameSession::OnEnterWaitQueue(const uint64 myTicket)
{
}

void GameSession::OnDisconnecting(const eDisconnectReason reason)
{
}

void GameSession::OnDisconnected()
{
}

void GameSession::OnSend(const int32 len)
{
}

void GameSession::OnRecvPacket(byte* pBuffer, const int32 len)
{
	PacketSessionRef pSession = GetPacketSessionRef();

	if (PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, pSession) == false)
	{
		pSession->Disconnect(eDisconnectReason::Kicked);
	}
}

void GameSession::OnError(const int32 errorCode)
{
}