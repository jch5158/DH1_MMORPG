#include "pch.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "MySqlService.h"
#include "GatewayService.h"
#include "PacketServiceTypeHandler.h"
#include "AccountTable.h"

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

		// DB: is_online = false, last_logout = now
		MySqlServiceRef pMySqlService = ISingleton<GatewayService>::GetInstance().GetMySqlServiceRef();
		if (pMySqlService != nullptr)
		{
			const uint64 accountId = mAccountId;
			pMySqlService->ExecuteAsync([accountId](sqlpp::mysql::connection& db)
				{
					try
					{
						const db::Account account{};
						db(sqlpp::update(account)
							.set(account.isOnline = false, account.lastLogout = std::chrono::system_clock::now())
							.where(account.accountId == static_cast<int64>(accountId)));
					}
					catch (const sqlpp::exception& e)
					{
						NET_ENGINE_LOG_ERROR("ClientSession::OnDisconnected - DB update failed, accountId: {}, error: {}", accountId, e.what());
					}
				});
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
