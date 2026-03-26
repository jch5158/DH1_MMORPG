#include "pch.h"
#include "GatewaySession.h"
#include "PacketHandler/PacketServiceTypeHandler.h"
#include "PacketHandler/GameSessionPacketHandler.h"
#include "GridManager.h"
#include "GameSessionManager.h"
#include "PlayerObject.h"
#include "RealmSession.h"
#include "WorldService.h"

GatewaySession::GatewaySession(const int32 receiveBufferSize, const int32 maxPacketSize)
	: PacketSession(receiveBufferSize, maxPacketSize)
{
}

void GatewaySession::OnConnected()
{
	NET_ENGINE_LOG_INFO("GatewaySession::OnConnected - GatewayServer connected, sessionId: {}", GetSessionId());
}

void GatewaySession::OnDisconnecting(const eDisconnectReason reason)
{
	NET_ENGINE_LOG_INFO("GatewaySession::OnDisconnecting - sessionId: {}, reason: {}", GetSessionId(), static_cast<int32>(reason));
}

void GatewaySession::OnDisconnected()
{
	NET_ENGINE_LOG_WARN("GatewaySession::OnDisconnected - GatewayServer disconnected, sessionId: {}, gatewayServerId: {}", GetSessionId(), mGatewayServerId);

	// GatewayServer 끊김 시 해당 게이트웨이에 속한 모든 오브젝트 + 세션 제거
	if (mGatewayServerId != 0)
	{
		auto& worldService = ISingleton<WorldService>::GetInstance();

		// GridManager에서 해당 게이트웨이의 오브젝트 벌크 제거
		const auto pGridManager = worldService.GetGridManagerRef();
		if (pGridManager != nullptr)
		{
			Vector<uint64> removedAccountIds;
			pGridManager->RemoveObjectsByGateway(mGatewayServerId, removedAccountIds);

			if (!removedAccountIds.empty())
			{
				NET_ENGINE_LOG_INFO("GatewaySession::OnDisconnected - Removed {} objects for gatewayServerId: {}", removedAccountIds.size(), mGatewayServerId);
			}
		}

		// GameSessionManager에서도 벌크 제거
		const auto pGameSessionManager = worldService.GetGameSessionManagerRef();
		if (pGameSessionManager != nullptr)
		{
			const Vector<uint64> removedAccountIds = pGameSessionManager->RemoveSessionsByGateway(mGatewayServerId);

			// RealmServer에 이탈 동기화
			if (!removedAccountIds.empty())
			{
				const auto pRealmClientService = worldService.GetRealmClientServiceRef();
				if (pRealmClientService != nullptr)
				{
					const auto pRealmSession = std::static_pointer_cast<RealmSession>(pRealmClientService->GetFirstSessionRef());
					if (pRealmSession != nullptr)
					{
						for (const uint64 accountId : removedAccountIds)
						{
							Protocol::S2S_GAME_SESSION_SYNC_LEAVE_NOT syncPacket;
							syncPacket.set_accountid(accountId);
							syncPacket.set_worldserverid(worldService.GetWorldServerId());

							pRealmSession->Send(GameSessionPacketHandler::MakeSendBuffer(syncPacket));
						}
					}
				}
			}
		}
	}
}

void GatewaySession::OnSend(const int32 len)
{
}

void GatewaySession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, GetPacketSessionRef());
}

void GatewaySession::UpdateHeartbeat(const int32 serverId)
{
	mGatewayServerId = serverId;
	mLastHeartbeatMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count(), std::memory_order_release);
}

int64 GatewaySession::GetLastHeartbeatMs() const
{
	return mLastHeartbeatMs.load(std::memory_order_acquire);
}

int32 GatewaySession::GetGatewayServerId() const
{
	return mGatewayServerId;
}
