#include "pch.h"
#include "GameTickProcessor.h"
#include "GridManager.h"
#include "GridCell.h"
#include "GameObject.h"
#include "PlayerObject.h"
#include "WorldService.h"
#include "GatewaySession.h"
#include "PacketHandler/GameSessionPacketHandler.h"
#include "Movement.pb.h"
#include "PacketId.h"

namespace
{
	constexpr uint32 MAKE_MOVEMENT_HEADER_ID(const uint16 packetId)
	{
		return (static_cast<uint32>(Protocol::eServiceType::SERVICE_TYPE_MOVEMENT) << 16) | static_cast<uint32>(packetId);
	}

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
		header->id = MAKE_MOVEMENT_HEADER_ID(packetId);
		packet.SerializeToArray(&header[1], dataSize);
		sendBuffer->Commit(packetSize);

		return sendBuffer;
	}
}

GameTickProcessor::GameTickProcessor(GridManagerRef pGridManager, const float moveSpeed)
	: mpGridManager(std::move(pGridManager))
	, mMoveSpeed(moveSpeed)
{
}

void GameTickProcessor::ProcessTick(const int64 deltaMs)
{
	const float deltaSec = static_cast<float>(deltaMs) / 1000.0f;

	processMovementInputs();
	updatePositions(deltaSec);
	sendMoveResults();
	processVisibilityChanges();
	broadcastSnapshots();

	mMovedActorIds.clear();
}

void GameTickProcessor::EnqueueMoveInput(const MoveInputEntry& entry)
{
	std::lock_guard<Mutex> lock(mInputQueueLock);
	mPendingInputs.push_back(entry);
}

void GameTickProcessor::processMovementInputs()
{
	{
		std::lock_guard<Mutex> lock(mInputQueueLock);
		mProcessingInputs.swap(mPendingInputs);
		mPendingInputs.clear();
	}

	for (const auto& input : mProcessingInputs)
	{
		const auto pObject = mpGridManager->GetObjectByAccountId(input.mAccountId);
		if (pObject == nullptr || pObject->GetObjectType() != eGameObjectType::Player)
		{
			continue;
		}

		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
		pPlayer->UpdateMoveInput(input.mSequenceId, input.mDirectionX, input.mDirectionY, input.mDirectionZ,
			input.mRotationYaw, input.mClientTimestamp, input.mPositionZ);

		mMovedActorIds.push_back(pPlayer->GetId());

		NET_ENGINE_LOG_TRACE("GameTick - Input processed, accountId: {}, seq: {}, vel: ({:.1f}, {:.1f}, {:.1f})",
			input.mAccountId, input.mSequenceId, pPlayer->GetVelocityX(), pPlayer->GetVelocityY(), pPlayer->GetVelocityZ());
	}

	mProcessingInputs.clear();
}

void GameTickProcessor::updatePositions(const float deltaSec)
{
	const auto allObjects = mpGridManager->GetAllObjects();

	for (const auto& pObject : allObjects)
	{
		if (!pObject->IsMoving())
		{
			continue;
		}

		// 속도 상한 검증
		const float speedSq = pObject->GetVelocityX() * pObject->GetVelocityX()
			+ pObject->GetVelocityY() * pObject->GetVelocityY()
			+ pObject->GetVelocityZ() * pObject->GetVelocityZ();
		const float maxSpeed = mMoveSpeed * 1.5f;
		if (speedSq > maxSpeed * maxSpeed)
		{
			const float speed = std::sqrt(speedSq);
			const float scale = maxSpeed / speed;
			pObject->SetVelocity(
				pObject->GetVelocityX() * scale,
				pObject->GetVelocityY() * scale,
				pObject->GetVelocityZ() * scale);
		}

		// 위치 갱신 (Z는 클라이언트에서 직접 동기화하므로 velocity 기반 갱신하지 않음)
		pObject->SetPosition(
			pObject->GetPositionX() + pObject->GetVelocityX() * deltaSec,
			pObject->GetPositionY() + pObject->GetVelocityY() * deltaSec,
			pObject->GetPositionZ());

		NET_ENGINE_LOG_TRACE("GameTick - Position updated, actorId: {}, pos: ({:.1f}, {:.1f}, {:.1f})",
			pObject->GetId(), pObject->GetPositionX(), pObject->GetPositionY(), pObject->GetPositionZ());
	}
}

void GameTickProcessor::sendMoveResults()
{
	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	for (const uint64 actorId : mMovedActorIds)
	{
		const auto pObject = mpGridManager->GetObjectByActorId(actorId);
		if (pObject == nullptr || pObject->GetObjectType() != eGameObjectType::Player)
		{
			continue;
		}

		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);

		Protocol::S2C_MOVE_RESULT_NOT result;
		result.set_sequenceid(pPlayer->GetLastSequenceId());
		result.mutable_position()->set_x(pPlayer->GetPositionX());
		result.mutable_position()->set_y(pPlayer->GetPositionY());
		result.mutable_position()->set_z(pPlayer->GetPositionZ());
		result.mutable_velocity()->set_x(pPlayer->GetVelocityX());
		result.mutable_velocity()->set_y(pPlayer->GetVelocityY());
		result.mutable_velocity()->set_z(pPlayer->GetVelocityZ());
		result.set_rotationyaw(pPlayer->GetRotationYaw());
		result.set_servertimestamp(nowMs);

		sendRelayToClient(pPlayer->GetGatewaySessionId(), pPlayer->GetGatewayServerId(),
			MakeMovementSendBuffer(result, packet_id::S2C_MOVE_RESULT_NOT));
	}
}

void GameTickProcessor::processVisibilityChanges()
{
	const auto allObjects = mpGridManager->GetAllObjects();

	for (const auto& pObject : allObjects)
	{
		const int32 newCellX = mpGridManager->ComputeCellX(pObject->GetPositionX());
		const int32 newCellY = mpGridManager->ComputeCellY(pObject->GetPositionY());

		const auto pOldCell = mpGridManager->GetCell(
			mpGridManager->ComputeCellX(pObject->GetPositionX() - pObject->GetVelocityX() * 0.05f),
			mpGridManager->ComputeCellY(pObject->GetPositionY() - pObject->GetVelocityY() * 0.05f));

		// 현재 셀 계산
		const auto pCurrentCell = mpGridManager->GetCell(newCellX, newCellY);

		// GridCellId와 실제 위치 기반 셀이 다르면 이동
		const auto pAssignedCell = mpGridManager->GetCell(
			mpGridManager->ComputeCellX(pObject->GetPositionX()),
			mpGridManager->ComputeCellY(pObject->GetPositionY()));

		if (pAssignedCell == nullptr || pAssignedCell->GetId() != pObject->GetGridCellId())
		{
			// GridManager가 셀 간 이동 처리
			mpGridManager->MoveObjectToCell(pObject->GetId(), newCellX, newCellY);

			// 셀 전환 시 Enter/Leave 이벤트는 플레이어에게만 발송
			if (pObject->GetObjectType() != eGameObjectType::Player)
			{
				continue;
			}

			const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);

			// 새 셀 주변의 모든 오브젝트를 ENTER로 보내고
			// 기존 셀 주변에만 있던 오브젝트를 LEAVE로 보냄
			// (간소화: 셀 전환 시 주변 전체를 재전송)
			const auto nearbyObjects = mpGridManager->GetObjectsInRange(newCellX, newCellY);

			Protocol::S2C_ENTITY_ENTER_NOT enterPacket;
			for (const auto& pNearby : nearbyObjects)
			{
				if (pNearby->GetId() == pObject->GetId())
				{
					continue;
				}

				auto* pEntity = enterPacket.add_entities();
				pEntity->set_entityid(pNearby->GetId());
				pEntity->mutable_position()->set_x(pNearby->GetPositionX());
				pEntity->mutable_position()->set_y(pNearby->GetPositionY());
				pEntity->mutable_position()->set_z(pNearby->GetPositionZ());
				pEntity->mutable_velocity()->set_x(pNearby->GetVelocityX());
				pEntity->mutable_velocity()->set_y(pNearby->GetVelocityY());
				pEntity->mutable_velocity()->set_z(pNearby->GetVelocityZ());
				pEntity->set_rotationyaw(pNearby->GetRotationYaw());
			}

			if (enterPacket.entities_size() > 0)
			{
				sendRelayToClient(pPlayer->GetGatewaySessionId(), pPlayer->GetGatewayServerId(),
					MakeMovementSendBuffer(enterPacket, packet_id::S2C_ENTITY_ENTER_NOT));
			}
		}
	}
}

void GameTickProcessor::broadcastSnapshots()
{
	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	const auto allObjects = mpGridManager->GetAllObjects();

	for (const auto& pObject : allObjects)
	{
		if (pObject->GetObjectType() != eGameObjectType::Player)
		{
			continue;
		}

		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
		const int32 cellX = mpGridManager->ComputeCellX(pPlayer->GetPositionX());
		const int32 cellY = mpGridManager->ComputeCellY(pPlayer->GetPositionY());

		const auto nearbyObjects = mpGridManager->GetObjectsInRange(cellX, cellY);

		Protocol::S2C_ENTITY_SNAPSHOT_NOT snapshot;
		snapshot.set_servertimestamp(nowMs);

		for (const auto& pNearby : nearbyObjects)
		{
			if (pNearby->GetId() == pPlayer->GetId() || !pNearby->IsMoving())
			{
				continue;
			}

			auto* pEntity = snapshot.add_entities();
			pEntity->set_entityid(pNearby->GetId());
			pEntity->mutable_position()->set_x(pNearby->GetPositionX());
			pEntity->mutable_position()->set_y(pNearby->GetPositionY());
			pEntity->mutable_position()->set_z(pNearby->GetPositionZ());
			pEntity->mutable_velocity()->set_x(pNearby->GetVelocityX());
			pEntity->mutable_velocity()->set_y(pNearby->GetVelocityY());
			pEntity->mutable_velocity()->set_z(pNearby->GetVelocityZ());
			pEntity->set_rotationyaw(pNearby->GetRotationYaw());
			pEntity->set_timestamp(nowMs);
		}

		if (snapshot.entities_size() > 0)
		{
			sendRelayToClient(pPlayer->GetGatewaySessionId(), pPlayer->GetGatewayServerId(),
				MakeMovementSendBuffer(snapshot, packet_id::S2C_ENTITY_SNAPSHOT_NOT));
		}
	}
}

void GameTickProcessor::sendRelayToClient(const uint64 gatewaySessionId, const int32 gatewayServerId, const NetSendBufferRef& pInnerPacket)
{
	auto& worldService = ISingleton<WorldService>::GetInstance();
	const auto pServerService = worldService.GetServerServiceRef();
	if (pServerService == nullptr)
	{
		return;
	}

	const Vector<SessionRef> sessions = pServerService->GetActiveSessions();
	for (const auto& pSession : sessions)
	{
		const auto pGatewaySession = std::static_pointer_cast<GatewaySession>(pSession);
		if (pGatewaySession == nullptr || pGatewaySession->GetGatewayServerId() != gatewayServerId)
		{
			continue;
		}

		Protocol::S2S_RELAY_TO_CLIENT_NOT relay;
		relay.set_gatewaysessionid(gatewaySessionId);
		relay.set_payload(pInnerPacket->GetReadPtr(), pInnerPacket->GetUseSize());

		pGatewaySession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));
		break;
	}
}
