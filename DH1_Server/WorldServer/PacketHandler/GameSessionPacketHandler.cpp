#include "pch.h"
#include "GameSessionPacketHandler.h"
#include "GatewaySession.h"
#include "RealmSession.h"
#include "GameSessionManager.h"
#include "GridManager.h"
#include "PlayerObject.h"
#include "WorldService.h"
#include "MySqlService.h"
#include "Table/PlayerCharacterTable.h"
#include "PacketHandler/PacketServiceTypeHandler.h"
#include "PacketHandler/MovementPacketHandler.h"
#include "Movement.pb.h"
#include "RelayContext.h"

namespace
{
	template<typename T>
	NetSendBufferRef MakeMovementSendBuffer(const T& packet, const uint16 packetId)
	{
		const uint16 dataSize = static_cast<uint16>(packet.ByteSizeLong());
		const uint16 packetSize = dataSize + static_cast<uint16>(sizeof(PacketHeader));

		auto sendBuffer = cpp_net_engine::MakeSendBuffer(packetSize);
		byte* pBuffer = sendBuffer->Reserve(packetSize);
		if (pBuffer == nullptr)
		{
			return nullptr;
		}

		auto* header = reinterpret_cast<PacketHeader*>(pBuffer);
		header->size = packetSize;
		header->id = (static_cast<uint32>(Protocol::eServiceType::SERVICE_TYPE_MOVEMENT) << 16) | static_cast<uint32>(packetId);
		packet.SerializeToArray(&header[1], dataSize);
		sendBuffer->Commit(packetSize);

		return sendBuffer;
	}
}

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

			// Gateway에 결과 응답
			pSession->Send(GameSessionPacketHandler::MakeSendBuffer(response));

			// 기존 PlayerObject의 gateway 정보 갱신
			const auto pGridManager = worldService.GetGridManagerRef();
			if (pGridManager != nullptr)
			{
				const auto pObject = pGridManager->GetObjectByAccountId(accountId);
				if (pObject != nullptr)
				{
					const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
					pPlayer->SetGatewayInfo(gatewaySessionId, gatewayServerId);

					// 재접속 시 저장된 위치를 클라이언트에 전송
					Protocol::S2C_SPAWN_POSITION_RES reconnectSpawnRes;
					reconnectSpawnRes.mutable_position()->set_x(pPlayer->GetPositionX());
					reconnectSpawnRes.mutable_position()->set_y(pPlayer->GetPositionY());
					reconnectSpawnRes.mutable_position()->set_z(pPlayer->GetPositionZ());
					reconnectSpawnRes.set_rotationyaw(pPlayer->GetRotationYaw());

					auto reconnectInnerBuffer = MakeMovementSendBuffer(reconnectSpawnRes, packet_id::S2C_SPAWN_POSITION_RES);
					Protocol::S2S_RELAY_TO_CLIENT_NOT reconnectRelay;
					reconnectRelay.set_gatewaysessionid(gatewaySessionId);
					reconnectRelay.set_payload(reconnectInnerBuffer->GetReadPtr(), reconnectInnerBuffer->GetUseSize());
					pSession->Send(GameSessionPacketHandler::MakeSendBuffer(reconnectRelay));

					NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Reconnect spawn position sent to client, accountId: {}, pos: ({}, {}, {})",
						accountId, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ());
				}
			}

			// RealmServer에 동기화
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

			return true;
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

	// 성공 시 DB 로드 → PlayerObject 생성 → RealmServer 동기화
	if (response.result() == Protocol::GAME_SESSION_SUCCESS)
	{
		const auto pMySqlService = worldService.GetMySqlServiceRef();
		const auto pGridManager = worldService.GetGridManagerRef();
		const int32 worldServerId = worldService.GetWorldServerId();
		const float defaultMoveSpeed = worldService.GetDefaultMoveSpeed();
		const auto pRealmClientService = worldService.GetRealmClientServiceRef();

		if (pMySqlService != nullptr && pGridManager != nullptr)
		{
			pMySqlService->ExecuteAsync(accountId, [accountId, gatewaySessionId, gatewayServerId, worldServerId,
				defaultMoveSpeed, pGridManager, pRealmClientService, pSession](sqlpp::mysql::connection& db)
				{
					const db::PlayerCharacter table{};

					float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
					float rotYaw = 0.0f;
					int32 level = 1;
					int64 experience = 0;

					// DB에서 캐릭터 조회
					auto result = db(sqlpp::select(sqlpp::all_of(table)).from(table).where(
						table.accountId == static_cast<int64>(accountId)));

					if (!result.empty())
					{
						const auto& row = result.front();
						posX = static_cast<float>(row.positionX);
						posY = static_cast<float>(row.positionY);
						posZ = static_cast<float>(row.positionZ);
						rotYaw = static_cast<float>(row.rotationYaw);
						level = static_cast<int32>(row.level);
						experience = static_cast<int64>(row.experience);

						// Z=0이면 기본 스폰 높이로 보정
						if (posZ < 1.0f)
						{
							posZ = 148.0f;
						}

						// last_played_at 갱신
						db(sqlpp::update(table).set(
							table.lastPlayedAt = std::chrono::system_clock::now()
						).where(table.accountId == static_cast<int64>(accountId)));

						NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Character loaded from DB, accountId: {}, pos: ({}, {}, {}), level: {}",
							accountId, posX, posY, posZ, level);
					}
					else
					{
						// 새 캐릭터 생성
						const std::string charName = "Player_" + std::to_string(accountId);

						constexpr double defaultSpawnZ = 148.0;

						db(sqlpp::insert_into(table).set(
							table.accountId = static_cast<int64>(accountId),
							table.characterName = charName,
							table.level = 1,
							table.experience = static_cast<int64>(0),
							table.positionX = 0.0,
							table.positionY = 0.0,
							table.positionZ = defaultSpawnZ,
							table.rotationYaw = 0.0,
							table.worldId = worldServerId
						));

						posZ = static_cast<float>(defaultSpawnZ);

						NET_ENGINE_LOG_INFO("GameSessionPacketHandler - New character created in DB, accountId: {}, name: {}", accountId, charName);
					}

					// PlayerObject 생성 (DB 위치 반영)
					auto pPlayerObject = cpp_net_engine::MakeShared<PlayerObject>(accountId, gatewaySessionId, gatewayServerId);
					pPlayerObject->SetMoveSpeed(defaultMoveSpeed);
					pPlayerObject->SetPosition(posX, posY, posZ);
					pPlayerObject->SetRotationYaw(rotYaw);

					const int32 spawnCellX = pGridManager->ComputeCellX(posX);
					const int32 spawnCellY = pGridManager->ComputeCellY(posY);
					pGridManager->AddObject(pPlayerObject, spawnCellX, spawnCellY);

					NET_ENGINE_LOG_INFO("GameSessionPacketHandler - PlayerObject created, accountId: {}, actorId: {}, pos: ({}, {}, {})",
						accountId, pPlayerObject->GetId(), posX, posY, posZ);

						// DB 로드 후 초기 위치를 클라이언트에 전송
						Protocol::S2C_SPAWN_POSITION_RES spawnRes;
						spawnRes.mutable_position()->set_x(posX);
						spawnRes.mutable_position()->set_y(posY);
						spawnRes.mutable_position()->set_z(posZ);
						spawnRes.set_rotationyaw(rotYaw);

						auto spawnInnerBuffer = MakeMovementSendBuffer(spawnRes, packet_id::S2C_SPAWN_POSITION_RES);
						Protocol::S2S_RELAY_TO_CLIENT_NOT spawnRelay;
						spawnRelay.set_gatewaysessionid(gatewaySessionId);
						spawnRelay.set_payload(spawnInnerBuffer->GetReadPtr(), spawnInnerBuffer->GetUseSize());
						pSession->Send(GameSessionPacketHandler::MakeSendBuffer(spawnRelay));

						NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Initial spawn position sent to client, accountId: {}, pos: ({}, {}, {}), yaw: {}",
							accountId, posX, posY, posZ, rotYaw);

					// RealmServer에 동기화
					if (pRealmClientService != nullptr)
					{
						const auto pRealmSession = std::static_pointer_cast<RealmSession>(pRealmClientService->GetFirstSessionRef());
						if (pRealmSession != nullptr)
						{
							Protocol::S2S_GAME_SESSION_SYNC_ENTER_NOT syncPacket;
							syncPacket.set_accountid(accountId);
							syncPacket.set_worldserverid(worldServerId);
							syncPacket.set_gatewayserverid(gatewayServerId);

							pRealmSession->Send(GameSessionPacketHandler::MakeSendBuffer(syncPacket));
						}
					}
				});
		}
	}

	return true;
}

bool GameSessionPacketHandler::HANDLE_S2S_GAME_SESSION_LEAVE_NOT(const Protocol::S2S_GAME_SESSION_LEAVE_NOT& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = packet.accountid();
	const int32 reason = packet.reason();

	auto& worldService = ISingleton<WorldService>::GetInstance();
	const auto pGameSessionManager = worldService.GetGameSessionManagerRef();
	if (pGameSessionManager == nullptr)
	{
		return false;
	}

	// 중복 로그인: 위치 저장 후 Pending 상태로 전환 (재연결 대기)
	if (reason == static_cast<int32>(Protocol::eSessionLeaveReason::SESSION_LEAVE_DUPLICATE_LOGIN))
	{
		// 현재 위치를 DB에 저장
		const auto pGridManager = worldService.GetGridManagerRef();
		if (pGridManager != nullptr)
		{
			const auto pObject = pGridManager->GetObjectByAccountId(accountId);
			if (pObject != nullptr)
			{
				const float posX = pObject->GetPositionX();
				const float posY = pObject->GetPositionY();
				const float posZ = pObject->GetPositionZ();
				const float rotYaw = pObject->GetRotationYaw();

				const auto pMySqlService = worldService.GetMySqlServiceRef();
				if (pMySqlService != nullptr)
				{
					pMySqlService->ExecuteAsync(accountId, [accountId, posX, posY, posZ, rotYaw](sqlpp::mysql::connection& db)
						{
							const db::PlayerCharacter table{};
							db(sqlpp::update(table).set(
								table.positionX = static_cast<double>(posX),
								table.positionY = static_cast<double>(posY),
								table.positionZ = static_cast<double>(posZ),
								table.rotationYaw = static_cast<double>(rotYaw),
								table.lastPlayedAt = std::chrono::system_clock::now()
							).where(table.accountId == static_cast<int64>(accountId)));

							NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Position saved before Pending, accountId: {}, pos: ({}, {}, {})",
								accountId, posX, posY, posZ);
						});
				}
			}
		}

		if (pGameSessionManager->SetSessionPending(accountId))
		{
			NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Session set to Pending (DuplicateLogin), accountId: {}", accountId);
		}
		else
		{
			NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Session not found for Pending, accountId: {}", accountId);
		}

		return true;
	}

	// 일반 이탈: DB에 위치 저장 → PlayerObject + 세션 제거
	const auto pGridManager = worldService.GetGridManagerRef();
	if (pGridManager != nullptr)
	{
		const auto pObject = pGridManager->GetObjectByAccountId(accountId);
		if (pObject != nullptr)
		{
			// DB에 현재 위치 저장
			const float posX = pObject->GetPositionX();
			const float posY = pObject->GetPositionY();
			const float posZ = pObject->GetPositionZ();
			const float rotYaw = pObject->GetRotationYaw();

			NET_ENGINE_LOG_INFO("GameSessionPacketHandler - LEAVE saving pos: ({}, {}, {}), accountId: {}",
				posX, posY, posZ, accountId);
			const auto pMySqlService = worldService.GetMySqlServiceRef();
			if (pMySqlService != nullptr)
			{
				pMySqlService->ExecuteAsync(accountId, [accountId, posX, posY, posZ, rotYaw, pMySqlService](sqlpp::mysql::connection& db)
					{
						const db::PlayerCharacter table{};
						db(sqlpp::update(table).set(
							table.positionX = static_cast<double>(posX),
							table.positionY = static_cast<double>(posY),
							table.positionZ = static_cast<double>(posZ),
							table.rotationYaw = static_cast<double>(rotYaw),
							table.lastPlayedAt = std::chrono::system_clock::now()
						).where(table.accountId == static_cast<int64>(accountId)));

						NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Character position saved, accountId: {}, pos: ({}, {}, {})",
							accountId, posX, posY, posZ);
						// DB 저장 완료 후 라우팅 키 해제 (재로그인 LOAD와 순서 보장)
						pMySqlService->ReleaseRoutingKey(accountId);
					});
			}

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
