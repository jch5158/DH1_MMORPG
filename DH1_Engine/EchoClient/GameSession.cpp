#include "pch.h"
#include "PacketHandler/PacketServiceTypeHandler.h"
#include "GameSession.h"

#include "Player.h"
#include "PlayerManager.h"
#include "NetService.h"

GameSession::GameSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	:PacketSession(receiveBufferSize, maxPacketSize)
	, mbDisconnectRequested(false)
{
}

void GameSession::OnConnected()
{
	mbDisconnectRequested.store(false);

	Protocol::C2S_ECHO_REQ packet;
	packet.set_echomsg(ECHO_TEST_MESSAGE);
	const auto pSendBuffer = EchoPacketHandler::MakeSendBuffer(packet);
	Send(pSendBuffer);
}

void GameSession::OnDisconnecting(const eDisconnectReason reason)
{
	if (mbDisconnectRequested.load())
	{
		return;
	}

	switch (reason)
	{
	case eDisconnectReason::Closed:
	case eDisconnectReason::Timeout:
		break;
	default:
		NET_ENGINE_LOG_FATAL("[GameSession] Unexpected disconnect! sessionId: {}, reason: {}", GetSessionId(), static_cast<uint8>(reason));
		CrashReporter::Crash();
		break;
	}
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

