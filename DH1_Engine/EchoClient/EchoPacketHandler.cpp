#include "pch.h"
#include "PacketHandler/EchoPacketHandler.h"

#include "Player.h"
#include "PlayerManager.h"
#include "NetService.h"
#include "GameSession.h"

bool EchoPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
                                                 const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("EchoPacketHandler::HANDLE_PACKET_ID_INVALID\n");

	return false;
}

bool EchoPacketHandler::HANDLE_S2C_ECHO_RES(const Protocol::S2C_ECHO_RES& packet, const PacketSessionRef& pSession)
{
	if (packet.echomsg() != GameSession::ECHO_TEST_MESSAGE)
	{
		NET_ENGINE_LOG_FATAL("[EchoPacketHandler] Data mismatch! sessionId: {}, expected size: {}, received size: {}",
			pSession->GetSessionId(),
			std::strlen(GameSession::ECHO_TEST_MESSAGE),
			packet.echomsg().size());
		CrashReporter::Crash();
	}

	// 1% 확률로 의도적 Disconnect 후 재연결 테스트
	thread_local std::mt19937 rng(std::random_device{}());
	thread_local std::uniform_int_distribution<int32> dist(1, 100);

	if (dist(rng) == 1)
	{
		const auto pGameSession = std::static_pointer_cast<GameSession>(pSession);
		pGameSession->mbDisconnectRequested.store(true);
		pSession->Disconnect(eDisconnectReason::Closed);
		return true;
	}

	Protocol::C2S_ECHO_REQ retPacket;
	retPacket.set_echomsg(GameSession::ECHO_TEST_MESSAGE);

	const auto pSendBuffer = EchoPacketHandler::MakeSendBuffer(retPacket);

	pSession->Send(pSendBuffer);

	return true;
}
