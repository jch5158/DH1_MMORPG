#include "pch.h"
#include "PacketHandler/PacketServiceTypeHandler.h"
#include "GameSession.h"

#include "Player.h"
#include "PlayerManager.h"
#include "Service.h"

GameSession::GameSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	:PacketSession(receiveBufferSize, maxPacketSize)
{
}

void GameSession::OnConnected()
{
	Protocol::C2S_ECHO_REQ packet;
	packet.set_echomsg("Hello World\n");
	const auto pSendBuffer = EchoPacketHandler::MakeSendBuffer(packet);
	Send(pSendBuffer);
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

void GameSession::OnReceivePacket(const byte* pBuffer, const int32 len)
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