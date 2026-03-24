#include "pch.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "GatewayService.h"
#include "PacketServiceTypeHandler.h"

ClientSession::ClientSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	:PacketSession(receiveBufferSize, maxPacketSize)
{
}

void ClientSession::OnConnected()
{
	NET_ENGINE_LOG_INFO("ClientSession::OnConnected - sessionId: {}", GetSessionId());
}

void ClientSession::OnDisconnecting(const eDisconnectReason reason)
{
	NET_ENGINE_LOG_INFO("ClientSession::OnDisconnecting - sessionId: {}, reason: {}", GetSessionId(), static_cast<int32>(reason));
}

void ClientSession::OnDisconnected()
{
	NET_ENGINE_LOG_INFO("ClientSession::OnDisconnected - sessionId: {}, accountId: {}", GetSessionId(), mAccountId);

	if (mAccountId != 0)
	{
		const auto pManager = ISingleton<GatewayService>::GetInstance().GetClientSessionManagerRef();
		if (pManager != nullptr)
		{
			(void)pManager->RemoveClientSession(mAccountId);
		}

		mAccountId = 0;
	}
}

void ClientSession::OnSend(const int32 len)
{
}

void ClientSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketSessionRef pSession = GetPacketSessionRef();

	if (PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, pSession) == false)
	{
		pSession->Disconnect(eDisconnectReason::Kicked);
	}
}
