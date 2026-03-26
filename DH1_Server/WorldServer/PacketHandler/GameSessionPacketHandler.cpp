#include "pch.h"
#include "GameSessionPacketHandler.h"
#include "GatewaySession.h"
#include "RealmSession.h"
#include "GameSessionManager.h"
#include "WorldService.h"

bool GameSessionPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool GameSessionPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("GameSessionPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool GameSessionPacketHandler::HANDLE_S2S_GAME_SESSION_ENTER_NOT(const Protocol::S2S_GAME_SESSION_ENTER_NOT& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = packet.accountid();
	const uint64 gatewaySessionId = packet.gatewaysessionid();
	const int32 gatewayServerId = packet.gatewayserverid();

	auto& worldService = ISingleton<WorldService>::GetInstance();
	const auto pGameSessionManager = worldService.GetGameSessionManagerRef();
	if (pGameSessionManager == nullptr)
	{
		return false;
	}

	GameSessionInfo sessionInfo;
	sessionInfo.mAccountId = accountId;
	sessionInfo.mGatewaySessionId = gatewaySessionId;
	sessionInfo.mGatewayServerId = gatewayServerId;

	Protocol::S2S_GAME_SESSION_ENTER_RES response;
	response.set_accountid(accountId);

	// 동일 accountId가 이미 있으면 재접속으로 처리 (UpdateSession)
	if (pGameSessionManager->HasSession(accountId))
	{
		if (pGameSessionManager->UpdateSession(sessionInfo))
		{
			response.set_result(Protocol::GAME_SESSION_SUCCESS);
			NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Session updated (reconnect), accountId: {}, gatewaySessionId: {}, gatewayServerId: {}",
				accountId, gatewaySessionId, gatewayServerId);
		}
		else
		{
			response.set_result(Protocol::GAME_SESSION_FAIL_INTERNAL);
			NET_ENGINE_LOG_ERROR("GameSessionPacketHandler - Session update failed, accountId: {}", accountId);
		}
	}
	else
	{
		if (pGameSessionManager->AddSession(sessionInfo))
		{
			response.set_result(Protocol::GAME_SESSION_SUCCESS);
			NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Session added, accountId: {}, gatewaySessionId: {}, gatewayServerId: {}",
				accountId, gatewaySessionId, gatewayServerId);
		}
		else
		{
			response.set_result(Protocol::GAME_SESSION_FAIL_FULL);
			NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Session add failed (full), accountId: {}", accountId);
		}
	}

	// Gateway에 결과 응답
	pSession->Send(GameSessionPacketHandler::MakeSendBuffer(response));

	// 성공 시 RealmServer에 동기화
	if (response.result() == Protocol::GAME_SESSION_SUCCESS)
	{
		const auto pRealmClientService = worldService.GetRealmClientServiceRef();
		if (pRealmClientService != nullptr)
		{
			const auto pRealmSession = std::static_pointer_cast<RealmSession>(pRealmClientService->GetFirstSessionRef());
			if (pRealmSession != nullptr)
			{
				Protocol::S2S_GAME_SESSION_SYNC_ENTER_NOT syncPacket;
				syncPacket.set_accountid(accountId);
				syncPacket.set_worldserverid(worldService.GetWorldServerId());
				syncPacket.set_gatewayserverid(gatewayServerId);

				pRealmSession->Send(GameSessionPacketHandler::MakeSendBuffer(syncPacket));
			}
		}
	}

	return true;
}

bool GameSessionPacketHandler::HANDLE_S2S_GAME_SESSION_LEAVE_NOT(const Protocol::S2S_GAME_SESSION_LEAVE_NOT& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = packet.accountid();

	auto& worldService = ISingleton<WorldService>::GetInstance();
	const auto pGameSessionManager = worldService.GetGameSessionManagerRef();
	if (pGameSessionManager == nullptr)
	{
		return false;
	}

	if (pGameSessionManager->RemoveSession(accountId))
	{
		NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Session removed, accountId: {}", accountId);

		// RealmServer에 이탈 동기화
		const auto pRealmClientService = worldService.GetRealmClientServiceRef();
		if (pRealmClientService != nullptr)
		{
			const auto pRealmSession = std::static_pointer_cast<RealmSession>(pRealmClientService->GetFirstSessionRef());
			if (pRealmSession != nullptr)
			{
				Protocol::S2S_GAME_SESSION_SYNC_LEAVE_NOT syncPacket;
				syncPacket.set_accountid(accountId);
				syncPacket.set_worldserverid(worldService.GetWorldServerId());

				pRealmSession->Send(GameSessionPacketHandler::MakeSendBuffer(syncPacket));
			}
		}
	}
	else
	{
		NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Session not found for removal, accountId: {}", accountId);
	}

	return true;
}

bool GameSessionPacketHandler::HANDLE_S2S_RELAY_TO_WORLD_NOT(const Protocol::S2S_RELAY_TO_WORLD_NOT& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = packet.accountid();

	auto& worldService = ISingleton<WorldService>::GetInstance();
	const auto pGameSessionManager = worldService.GetGameSessionManagerRef();
	if (pGameSessionManager == nullptr)
	{
		return false;
	}

	if (!pGameSessionManager->HasSession(accountId))
	{
		NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Relay target session not found, accountId: {}", accountId);
		return true;
	}

	// TODO: payload에서 패킷을 추출하여 게임 로직 처리 (확장 포인트)
	NET_ENGINE_LOG_TRACE("GameSessionPacketHandler - Relay to world received, accountId: {}, payloadSize: {}",
		accountId, packet.payload().size());

	return true;
}
