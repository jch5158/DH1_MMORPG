#include "Network/CppNetEngine/NetSession.h"

#include "DH1_Client.h"
#include "Network/PacketHandler/PacketServiceTypeHandler.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

NetSession::NetSession(const int32 receiveBufferSize, const int32 maxPacketSize, AuthData ArgAuthData)
	:PacketSession(receiveBufferSize, maxPacketSize)
	, mAuthData(ArgAuthData)
{}

NetSession::~NetSession()
{
}

void NetSession::OnConnected()
{
	Protocol::C2S_LOGIN_REQ packet;
	packet.set_accountid(FCString::Atoi(*mAuthData.AccountId));
	packet.set_ticket(TCHAR_TO_UTF8(*mAuthData.Ticket));

	const auto SendBuffer = LoginPacketHandler::MakeSendBuffer(packet);
	Send(SendBuffer);
}

void NetSession::OnDisconnecting(const eDisconnectReason reason)
{
}

void NetSession::OnDisconnected()
{
}

void NetSession::OnSend(const int32 len)
{
}

void NetSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketSessionRef pSession = GetPacketSessionRef();

	if (pBuffer == nullptr || len < static_cast<int32>(sizeof(PacketHeader)))
	{
		pSession->Disconnect(eDisconnectReason::Kicked);
		return;
	}

	const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(pBuffer);
	if (static_cast<int32>(hdr->size) != len)
	{
		UE_LOG(LogDH1_Client, Error, TEXT("NetSession: 프레이밍 오류 — header.size(%u) != 수신 len(%d), 연결 종료"), hdr->size, len);
		pSession->Disconnect(eDisconnectReason::Kicked);
		return;
	}

	if (PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, pSession) == false)
	{
		pSession->Disconnect(eDisconnectReason::Kicked);
	}
}
