#include "pch.h"
#include "GameSession.h"
#include "Generated/PacketServiceTypeHandler.h"

GameSession::GameSession()
	:PacketSession()
{
}

GameSession::~GameSession()
{
}

void GameSession::OnConnected()
{
	fmt::print(L"Client Connect\n");
}

void GameSession::OnEnterWaitQueue(const uint64 myTicket)
{
}

void GameSession::OnDisconnecting(const eDisconnectReason reason)
{
	fmt::print(L"On Disconnecting : {}\n", static_cast<uint16>(reason));
}

void GameSession::OnDisconnected()
{
	fmt::print(L"Client Disconnect {}\n", GetSessionRef().use_count());
}

void GameSession::OnSend(const int32 len)
{
}

void GameSession::OnReceivePacket(byte* pBuffer, const int32 len)
{
	PacketSessionRef pSession = GetPacketSessionRef();

	if (PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, pSession) == false)
	{
		pSession->Disconnect(eDisconnectReason::Kicked);
	}
}

void GameSession::OnError(const int32 errorCode)
{
	fmt::print(L"OnError - errorCode : {}", errorCode);
}