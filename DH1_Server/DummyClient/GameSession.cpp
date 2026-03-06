#include "pch.h"
#include "Generated/PacketServiceTypeHandler.h"
#include "GameSession.h"

GameSession::GameSession()
	:PacketSession()
{
}

GameSession::~GameSession()
{
}

void GameSession::OnConnected()
{
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