#include "Network/CppNetEngine/NetSession.h"

#include "ISourceControlProvider.h"

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

void NetSession::OnError(const int32 errorCode)
{
}

void NetSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketSessionRef pSession = GetPacketSessionRef();

	if (PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, pSession) == false)
	{
		pSession->Disconnect(eDisconnectReason::Kicked);
	}
}
