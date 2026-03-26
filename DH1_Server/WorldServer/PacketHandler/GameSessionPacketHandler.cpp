#include "pch.h"
#include "GameSessionPacketHandler.h"
#include "GatewaySession.h"
#include "RealmSession.h"
#include "GameSessionManager.h"
#include "GridManager.h"
#include "PlayerObject.h"
#include "WorldService.h"
#include "PacketHandler/PacketServiceTypeHandler.h"
#include "PacketHandler/MovementPacketHandler.h"
#include "RelayContext.h"

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

	// 성공 시 PlayerObject 생성 + RealmServer에 동기화
	if (response.result() == Protocol::GAME_SESSION_SUCCESS)
	{
		// PlayerObject 생성 (스폰 위치 0,0,0)
		const auto pGridManager = worldService.GetGridManagerRef();
		if (pGridManager != nullptr)
		{
			auto pPlayerObject = cpp_net_engine::MakeShared<PlayerObject>(accountId, gatewaySessionId, gatewayServerId);
			pPlayerObject->SetMoveSpeed(worldService.GetDefaultMoveSpeed());

			const int32 spawnCellX = pGridManager->ComputeCellX(0.0f);
			const int32 spawnCellY = pGridManager->ComputeCellY(0.0f);
			pGridManager->AddObject(pPlayerObject, spawnCellX, spawnCellY);

			NET_ENGINE_LOG_INFO("GameSessionPacketHandler - PlayerObject created, accountId: {}, actorId: {}", accountId, pPlayerObject->GetId());
		}

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

	// PlayerObject 제거
	const auto pGridManager = worldService.GetGridManagerRef();
	if (pGridManager != nullptr)
	{
		const auto pObject = pGridManager->GetObjectByAccountId(accountId);
		if (pObject != nullptr)
		{
			pGridManager->RemoveObject(pObject->GetId());
		}
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

	const auto& payload = packet.payload();
	if (payload.size() < sizeof(PacketHeader))
	{
		NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Relay payload too small, accountId: {}", accountId);
		return true;
	}

	const auto* pHeader = reinterpret_cast<const PacketHeader*>(payload.data());
	const uint16 serviceType = PacketServiceTypeHandler::GET_SERVICE_TYPE(pHeader->id);
	const uint16 packetId = PacketServiceTypeHandler::GET_PACKET_ID(pHeader->id);
	const uint16 payloadSize = static_cast<uint16>(payload.size());

	// RelayContext에 accountId 설정 후 자동 생성된 핸들러 호출
	RelayContext::SetAccountId(accountId);

	switch (serviceType)
	{
	case static_cast<uint16>(Protocol::eServiceType::SERVICE_TYPE_MOVEMENT):
		MovementPacketHandler::HandlePacket(payloadSize, packetId,
			reinterpret_cast<const byte*>(payload.data()), pSession);
		break;
	default:
		NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Unknown relay service type: {}, accountId: {}", serviceType, accountId);
		break;
	}

	RelayContext::Clear();

	return true;
}
