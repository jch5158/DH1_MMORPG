#include "pch.h"
#include "MovementPacketHandler.h"
#include "GameTickProcessor.h"
#include "GridManager.h"
#include "PlayerObject.h"
#include "WorldService.h"
#include "MySqlService.h"
#include "RelayContext.h"
#include "PacketHandler/GameSessionPacketHandler.h"
#include "GameSessionManager.h"
#include "RealmSession.h"
#include "Table/PlayerCharacterTable.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <string>

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

bool MovementPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool MovementPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("MovementPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool MovementPacketHandler::HANDLE_C2S_MOVE_INPUT_NOT(const Protocol::C2S_MOVE_INPUT_NOT& packet, const PacketSessionRef& pSession)
{
	// WASD 이동 비활성화 — 클릭이동으로 통일
	return true;
}

bool MovementPacketHandler::HANDLE_C2S_SPAWN_POSITION_REQ(const Protocol::C2S_SPAWN_POSITION_REQ& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = RelayContext::GetAccountId();
	if (accountId == 0)
	{
		NET_ENGINE_LOG_ERROR("MovementPacketHandler - SPAWN_POSITION_REQ, RelayContext accountId is 0");
		return false;
	}

	auto& worldService = ISingleton<WorldService>::GetInstance();
	const auto pGridManager = worldService.GetGridManagerRef();
	if (pGridManager == nullptr)
	{
		return false;
	}

	const auto pObject = pGridManager->GetObjectByAccountId(accountId);
	if (pObject == nullptr)
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler - SPAWN_POSITION_REQ, PlayerObject not found, accountId: {}", accountId);
		return false;
	}

	const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
	const float posX = pPlayer->GetPositionX();
	const float posY = pPlayer->GetPositionY();
	const float posZ = pPlayer->GetPositionZ();
	const float rotYaw = pPlayer->GetRotationYaw();

	// S2C_SPAWN_POSITION_RES → RELAY_TO_CLIENT_NOT
	Protocol::S2C_SPAWN_POSITION_RES response;
	response.mutable_position()->set_x(posX);
	response.mutable_position()->set_y(posY);
	response.mutable_position()->set_z(posZ);
	response.set_rotationyaw(rotYaw);
	pPlayer->WriteCharacterSheetTo(response);

	auto innerBuffer = MakeMovementSendBuffer(response, packet_id::S2C_SPAWN_POSITION_RES);

	Protocol::S2S_RELAY_TO_CLIENT_NOT relayPacket;
	relayPacket.set_gatewaysessionid(pPlayer->GetGatewaySessionId());
	relayPacket.set_payload(innerBuffer->GetReadPtr(), innerBuffer->GetUseSize());

	pSession->Send(GameSessionPacketHandler::MakeSendBuffer(relayPacket));

	NET_ENGINE_LOG_INFO("MovementPacketHandler - SPAWN_POSITION_RES sent, accountId: {}, displayNameLen: {}, pos: ({}, {}, {}), yaw: {}",
		accountId, pPlayer->GetDisplayName().size(), posX, posY, posZ, rotYaw);

	return true;
}

bool MovementPacketHandler::HANDLE_C2S_MOVE_TO_POSITION_REQ(const Protocol::C2S_MOVE_TO_POSITION_REQ& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = RelayContext::GetAccountId();
	if (accountId == 0)
	{
		NET_ENGINE_LOG_ERROR("MovementPacketHandler - MOVE_TO_POSITION_REQ, RelayContext accountId is 0");
		return false;
	}

	// 목적지 NaN/Inf 검증
	const float destX = packet.destination().x();
	const float destY = packet.destination().y();
	const float destZ = packet.destination().z();

	if (std::isnan(destX) || std::isnan(destY) || std::isnan(destZ) ||
		std::isinf(destX) || std::isinf(destY) || std::isinf(destZ))
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler (World) - Invalid destination, accountId: {}", accountId);
		return true;
	}

	const auto pGameTickProcessor = ISingleton<WorldService>::GetInstance().GetGameTickProcessorRef();
	if (pGameTickProcessor == nullptr)
	{
		return false;
	}

	MoveToPositionEntry entry;
	entry.mAccountId = accountId;
	entry.mSequenceId = packet.sequenceid();
	entry.mDestinationX = destX;
	entry.mDestinationY = destY;
	entry.mDestinationZ = destZ;
	entry.mClientTimestamp = packet.clienttimestamp();
	entry.mHasCurrentPosition = packet.has_currentposition();
	entry.mCurrentX = packet.currentposition().x();
	entry.mCurrentY = packet.currentposition().y();
	entry.mCurrentZ = packet.currentposition().z();

	pGameTickProcessor->EnqueueMoveToPosition(entry);

	NET_ENGINE_LOG_TRACE("MovementPacketHandler - MOVE_TO_POSITION_REQ received, accountId: {}, dest: ({:.1f}, {:.1f}, {:.1f})",
		accountId, destX, destY, destZ);

	return true;
}

namespace
{
	float ComputeDefaultMaxHp(const int32 level)
	{
		return 100.f + static_cast<float>(std::max(0, level - 1)) * 20.f;
	}

	void TrimAsciiWhitespaceInPlace(std::string& s)
	{
		while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
		{
			s.pop_back();
		}
		size_t i = 0;
		while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
		{
			++i;
		}
		if (i > 0)
		{
			s.erase(0, i);
		}
	}

	bool IsValidCharacterName(const std::string& name)
	{
		if (name.size() < 2 || name.size() > 16)
		{
			return false;
		}
		for (const unsigned char ch : name)
		{
			if (ch < 0x20)
			{
				return false;
			}
		}
		return true;
	}
}

bool MovementPacketHandler::HANDLE_C2S_CREATE_CHARACTER_REQ(const Protocol::C2S_CREATE_CHARACTER_REQ& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = RelayContext::GetAccountId();
	if (accountId == 0)
	{
		NET_ENGINE_LOG_ERROR("MovementPacketHandler - CREATE_CHARACTER_REQ, RelayContext accountId is 0");
		return false;
	}

	auto& worldService = ISingleton<WorldService>::GetInstance();
	const auto pGameSessionManager = worldService.GetGameSessionManagerRef();
	if (pGameSessionManager == nullptr || !pGameSessionManager->HasSession(accountId))
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler - CREATE_CHARACTER_REQ, session not found, accountId: {}", accountId);
		return false;
	}

	std::string charName = packet.charactername();
	TrimAsciiWhitespaceInPlace(charName);

	const auto sessionInfoOpt = pGameSessionManager->GetSession(accountId);
	if (!sessionInfoOpt.has_value())
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler - CREATE_CHARACTER_REQ, session info not found, accountId: {}", accountId);
		return false;
	}
	const uint64 gatewaySessionId = sessionInfoOpt->mGatewaySessionId;
	const int32 gatewayServerId = sessionInfoOpt->mGatewayServerId;

	auto sendResult = [&pSession, gatewaySessionId](Protocol::eCreateCharacterResult result, const std::string& msg)
	{
		Protocol::S2C_CREATE_CHARACTER_RES res;
		res.set_result(result);
		res.set_message(msg);
		auto innerBuf = MakeMovementSendBuffer(res, packet_id::S2C_CREATE_CHARACTER_RES);
		Protocol::S2S_RELAY_TO_CLIENT_NOT relay;
		relay.set_gatewaysessionid(gatewaySessionId);
		relay.set_payload(innerBuf->GetReadPtr(), innerBuf->GetUseSize());
		pSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));
	};

	if (charName.size() < 2)
	{
		sendResult(Protocol::CREATE_CHARACTER_FAIL_NAME_TOO_SHORT, reinterpret_cast<const char*>(u8"닉네임은 최소 2자 이상이어야 합니다."));
		return true;
	}
	if (charName.size() > 16)
	{
		sendResult(Protocol::CREATE_CHARACTER_FAIL_NAME_TOO_LONG, reinterpret_cast<const char*>(u8"닉네임은 16자 이하여야 합니다."));
		return true;
	}
	if (!IsValidCharacterName(charName))
	{
		sendResult(Protocol::CREATE_CHARACTER_FAIL_NAME_INVALID, reinterpret_cast<const char*>(u8"닉네임에 사용할 수 없는 문자가 포함되어 있습니다."));
		return true;
	}

	const auto pMySqlService = worldService.GetMySqlServiceRef();
	const auto pGridManager = worldService.GetGridManagerRef();
	const int32 worldServerId = worldService.GetWorldServerId();
	const float defaultMoveSpeed = worldService.GetDefaultMoveSpeed();
	const auto pRealmClientService = worldService.GetRealmClientServiceRef();

	if (pMySqlService == nullptr || pGridManager == nullptr)
	{
		sendResult(Protocol::CREATE_CHARACTER_FAIL_INTERNAL, reinterpret_cast<const char*>(u8"서버 내부 오류가 발생했습니다."));
		return true;
	}

	if (pGridManager->GetObjectByAccountId(accountId) != nullptr)
	{
		sendResult(Protocol::CREATE_CHARACTER_FAIL_ALREADY_EXISTS, reinterpret_cast<const char*>(u8"이미 캐릭터가 존재합니다."));
		return true;
	}

	pMySqlService->ExecuteAsync(accountId, [accountId, gatewaySessionId, gatewayServerId, worldServerId,
		defaultMoveSpeed, charName, pGridManager, pRealmClientService, pSession, pMySqlService](sqlpp::mysql::connection& db)
		{
			const db::PlayerCharacter table{};

			auto existing = db(sqlpp::select(table.characterId).from(table).where(
				table.accountId == static_cast<int64>(accountId)));
			if (!existing.empty())
			{
				Protocol::S2C_CREATE_CHARACTER_RES res;
				res.set_result(Protocol::CREATE_CHARACTER_FAIL_ALREADY_EXISTS);
				res.set_message(reinterpret_cast<const char*>(u8"이미 캐릭터가 존재합니다."));
				auto innerBuf = MakeMovementSendBuffer(res, packet_id::S2C_CREATE_CHARACTER_RES);
				Protocol::S2S_RELAY_TO_CLIENT_NOT relay;
				relay.set_gatewaysessionid(gatewaySessionId);
				relay.set_payload(innerBuf->GetReadPtr(), innerBuf->GetUseSize());
				pSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));
				return;
			}

			auto dupName = db(sqlpp::select(table.characterId).from(table).where(
				table.characterName == charName));
			if (!dupName.empty())
			{
				Protocol::S2C_CREATE_CHARACTER_RES res;
				res.set_result(Protocol::CREATE_CHARACTER_FAIL_NAME_DUPLICATE);
				res.set_message(reinterpret_cast<const char*>(u8"이미 사용 중인 닉네임입니다."));
				auto innerBuf = MakeMovementSendBuffer(res, packet_id::S2C_CREATE_CHARACTER_RES);
				Protocol::S2S_RELAY_TO_CLIENT_NOT relay;
				relay.set_gatewaysessionid(gatewaySessionId);
				relay.set_payload(innerBuf->GetReadPtr(), innerBuf->GetUseSize());
				pSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));
				return;
			}

			constexpr double defaultSpawnZ = 148.0;
			const float maxHp = ComputeDefaultMaxHp(1);
			const float currentHp = maxHp;

			db(sqlpp::insert_into(table).set(
				table.accountId = static_cast<int64>(accountId),
				table.characterName = charName,
				table.level = 1,
				table.currentHp = static_cast<double>(currentHp),
				table.maxHp = static_cast<double>(maxHp),
				table.experience = static_cast<int64>(0),
				table.positionX = 0.0,
				table.positionY = 0.0,
				table.positionZ = defaultSpawnZ,
				table.rotationYaw = 0.0,
				table.worldId = worldServerId
			));

			NET_ENGINE_LOG_INFO("MovementPacketHandler - Character created in DB, accountId: {}, name: {}", accountId, charName);

			{
				Protocol::S2C_CREATE_CHARACTER_RES res;
				res.set_result(Protocol::CREATE_CHARACTER_SUCCESS);
				res.set_message("Character created successfully!");
				auto innerBuf = MakeMovementSendBuffer(res, packet_id::S2C_CREATE_CHARACTER_RES);
				Protocol::S2S_RELAY_TO_CLIENT_NOT relay;
				relay.set_gatewaysessionid(gatewaySessionId);
				relay.set_payload(innerBuf->GetReadPtr(), innerBuf->GetUseSize());
				pSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));
			}

			const float posX = 0.f, posY = 0.f;
			const float posZ = static_cast<float>(defaultSpawnZ);
			const float rotYaw = 0.f;

			auto pPlayerObject = cpp_net_engine::MakeShared<PlayerObject>(accountId, gatewaySessionId, gatewayServerId);
			pPlayerObject->SetMoveSpeed(defaultMoveSpeed);
			pPlayerObject->SetPosition(posX, posY, posZ);
			pPlayerObject->SetRotationYaw(rotYaw);
			pPlayerObject->SetCharacterSheet(std::string(charName), 1, currentHp, maxHp);

			const int32 spawnCellX = pGridManager->ComputeCellX(posX);
			const int32 spawnCellY = pGridManager->ComputeCellY(posY);
			pGridManager->AddObject(pPlayerObject, spawnCellX, spawnCellY);

			if (const auto pTick = ISingleton<WorldService>::GetInstance().GetGameTickProcessorRef())
			{
				pTick->NotifySpawnedPlayerVisibleEntities(pPlayerObject);
				pTick->NotifyNearbyPlayersAboutEntity(pPlayerObject);
			}

			Protocol::S2C_SPAWN_POSITION_RES spawnRes;
			spawnRes.mutable_position()->set_x(posX);
			spawnRes.mutable_position()->set_y(posY);
			spawnRes.mutable_position()->set_z(posZ);
			spawnRes.set_rotationyaw(rotYaw);
			pPlayerObject->WriteCharacterSheetTo(spawnRes);

			auto spawnInnerBuf = MakeMovementSendBuffer(spawnRes, packet_id::S2C_SPAWN_POSITION_RES);
			Protocol::S2S_RELAY_TO_CLIENT_NOT spawnRelay;
			spawnRelay.set_gatewaysessionid(gatewaySessionId);
			spawnRelay.set_payload(spawnInnerBuf->GetReadPtr(), spawnInnerBuf->GetUseSize());
			pSession->Send(GameSessionPacketHandler::MakeSendBuffer(spawnRelay));

			NET_ENGINE_LOG_INFO("MovementPacketHandler - Spawn position sent after character creation, accountId: {}", accountId);
		});

	return true;
}
