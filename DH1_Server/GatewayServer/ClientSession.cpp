#include "pch.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "RedisService.h"
#include "MySqlService.h"
#include "GatewayService.h"
#include "PacketServiceTypeHandler.h"
#include "PacketHandler/GameSessionPacketHandler.h"

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
		if (mbDuplicateLoginHandled)
		{
			NET_ENGINE_LOG_INFO("ClientSession::OnDisconnected - DuplicateLogin already handled, skipping cleanup, accountId: {}", mAccountId);
			mAccountId = 0;
		}
		else
		{
			if (mWorldServerId != 0)
			{
				const auto pWorldClientService = ISingleton<GatewayService>::GetInstance().GetWorldClientServiceRef();
				if (pWorldClientService != nullptr)
				{
					const auto pWorldSession = pWorldClientService->GetFirstSessionRef();
					if (pWorldSession != nullptr)
					{
						Protocol::S2S_GAME_SESSION_LEAVE_NOT leavePacket;
						leavePacket.set_accountid(mAccountId);
						leavePacket.set_gatewaysessionid(GetSessionId());

						pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(leavePacket));

						NET_ENGINE_LOG_INFO("ClientSession::OnDisconnected - LEAVE_NOT sent, accountId: {}, worldServerId: {}", mAccountId, mWorldServerId);
					}
				}

				mWorldServerId = 0;
			}

			const auto pManager = ISingleton<GatewayService>::GetInstance().GetClientSessionManagerRef();
			if (pManager != nullptr)
			{
				(void)pManager->RemoveClientSession(mAccountId);
			}

			auto& gw = ISingleton<GatewayService>::GetInstance();
			if (const auto pMySql = gw.GetMySqlServiceRef())
			{
				pMySql->ReleaseRoutingKey(mAccountId);
			}
			if (const auto pAccountMySql = gw.GetAccountMySqlServiceRef())
			{
				pAccountMySql->ReleaseRoutingKey(mAccountId);
			}

			const int32 gatewayId = ISingleton<GatewayService>::GetInstance().GetGatewayId();
			RedisServiceRef pRedisService = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
			if (pRedisService != nullptr)
			{
				const uint64 accountId = mAccountId;
				pRedisService->CheckAndDeleteSessionAsync(accountId, gatewayId, [accountId](const bool isDeleted)
					{
						if (isDeleted)
						{
							NET_ENGINE_LOG_INFO("ClientSession::OnDisconnected - Redis session cleared, accountId: {}", accountId);
						}
					});
			}

			mAccountId = 0;
		}
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

void ClientSession::UpdateHeartbeat()
{
	mLastHeartbeatMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count(), std::memory_order_release);
}

int64 ClientSession::GetLastHeartbeatMs() const
{
	return mLastHeartbeatMs.load(std::memory_order_acquire);
}
