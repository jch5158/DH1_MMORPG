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

namespace
{
	void FillEntityCharacterSheet(Protocol::EntityState* pEntity, const GameObjectRef& pObject)
	{
		if (pObject->GetObjectType() != eGameObjectType::Player)
		{
			return;
		}
		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
		pEntity->set_displayname(pPlayer->GetDisplayName());
		pEntity->set_level(pPlayer->GetLevel());
		pEntity->set_currenthp(pPlayer->GetCurrentHp());
		pEntity->set_maxhp(pPlayer->GetMaxHp());
	}
}

GameTickProcessor::GameTickProcessor(GridManagerRef pGridManager, NavMeshManagerRef pNavMeshManager, const float moveSpeed)
	: mpGridManager(std::move(pGridManager))
	, mpNavMeshManager(std::move(pNavMeshManager))
	, mMoveSpeed(moveSpeed)
{
}

void GameTickProcessor::ProcessTick(const int64 deltaMs)
{
	const float deltaSec = static_cast<float>(deltaMs) / 1000.0f;

	processMoveToPositionRequests();
	updatePathFollowing(deltaSec);
	sendPathResults();
	processVisibilityChanges();
	broadcastSnapshots();

	mPendingPathResults.clear();
}

void GameTickProcessor::EnqueueMoveToPosition(const MoveToPositionEntry& entry)
{
	std::lock_guard<Mutex> lock(mMoveToQueueLock);
	mPendingMoveToInputs.push_back(entry);
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
				FillEntityCharacterSheet(pEntity, pNearby);
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

void GameTickProcessor::RelayChatToGatewayClient(const uint64 gatewaySessionId, const int32 gatewayServerId, const NetSendBufferRef& pInnerPacket)
{
	sendRelayToClient(gatewaySessionId, gatewayServerId, pInnerPacket);
}

void GameTickProcessor::NotifySpawnedPlayerVisibleEntities(const PlayerObjectRef& pPlayer)
{
	if (pPlayer == nullptr || mpGridManager == nullptr)
	{
		return;
	}

	const int32 cellX = mpGridManager->ComputeCellX(pPlayer->GetPositionX());
	const int32 cellY = mpGridManager->ComputeCellY(pPlayer->GetPositionY());
	const auto nearbyObjects = mpGridManager->GetObjectsInRange(cellX, cellY);

	Protocol::S2C_ENTITY_ENTER_NOT enterPacket;
	for (const auto& pNearby : nearbyObjects)
	{
		if (pNearby->GetId() == pPlayer->GetId())
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
		FillEntityCharacterSheet(pEntity, pNearby);
	}

	if (enterPacket.entities_size() > 0)
	{
		sendRelayToClient(pPlayer->GetGatewaySessionId(), pPlayer->GetGatewayServerId(),
			MakeMovementSendBuffer(enterPacket, packet_id::S2C_ENTITY_ENTER_NOT));
	}
}

void GameTickProcessor::NotifyNearbyPlayersAboutEntity(const GameObjectRef& pSubject)
{
	if (pSubject == nullptr || mpGridManager == nullptr)
	{
		return;
	}

	const int32 cellX = mpGridManager->ComputeCellX(pSubject->GetPositionX());
	const int32 cellY = mpGridManager->ComputeCellY(pSubject->GetPositionY());
	const auto nearbyObjects = mpGridManager->GetObjectsInRange(cellX, cellY);

	Protocol::S2C_ENTITY_ENTER_NOT enterPacket;
	auto* pEntity = enterPacket.add_entities();
	pEntity->set_entityid(pSubject->GetId());
	pEntity->mutable_position()->set_x(pSubject->GetPositionX());
	pEntity->mutable_position()->set_y(pSubject->GetPositionY());
	pEntity->mutable_position()->set_z(pSubject->GetPositionZ());
	pEntity->mutable_velocity()->set_x(pSubject->GetVelocityX());
	pEntity->mutable_velocity()->set_y(pSubject->GetVelocityY());
	pEntity->mutable_velocity()->set_z(pSubject->GetVelocityZ());
	pEntity->set_rotationyaw(pSubject->GetRotationYaw());
	FillEntityCharacterSheet(pEntity, pSubject);

	const auto enterBuffer = MakeMovementSendBuffer(enterPacket, packet_id::S2C_ENTITY_ENTER_NOT);
	if (enterBuffer == nullptr)
	{
		return;
	}

	for (const auto& pNearby : nearbyObjects)
	{
		if (pNearby->GetId() == pSubject->GetId() || pNearby->GetObjectType() != eGameObjectType::Player)
		{
			continue;
		}
		const auto pNearbyPlayer = std::static_pointer_cast<PlayerObject>(pNearby);
		sendRelayToClient(pNearbyPlayer->GetGatewaySessionId(), pNearbyPlayer->GetGatewayServerId(), enterBuffer);
	}
}

void GameTickProcessor::NotifyNearbyPlayersAboutEntityLeave(const GameObjectRef& pSubject)
{
	if (pSubject == nullptr || mpGridManager == nullptr)
	{
		return;
	}

	const int32 cellX = mpGridManager->ComputeCellX(pSubject->GetPositionX());
	const int32 cellY = mpGridManager->ComputeCellY(pSubject->GetPositionY());
	const auto nearbyObjects = mpGridManager->GetObjectsInRange(cellX, cellY);

	Protocol::S2C_ENTITY_LEAVE_NOT leavePacket;
	leavePacket.add_entityids(pSubject->GetId());

	const auto leaveBuffer = MakeMovementSendBuffer(leavePacket, packet_id::S2C_ENTITY_LEAVE_NOT);
	if (leaveBuffer == nullptr)
	{
		return;
	}

	for (const auto& pNearby : nearbyObjects)
	{
		if (pNearby->GetId() == pSubject->GetId() || pNearby->GetObjectType() != eGameObjectType::Player)
		{
			continue;
		}
		const auto pNearbyPlayer = std::static_pointer_cast<PlayerObject>(pNearby);
		sendRelayToClient(pNearbyPlayer->GetGatewaySessionId(), pNearbyPlayer->GetGatewayServerId(), leaveBuffer);
	}

	NET_ENGINE_LOG_INFO("[AOI] EntityLeave broadcast for entityId={}, nearby_count={}", pSubject->GetId(), nearbyObjects.size());
}

void GameTickProcessor::sendAoiLeavesAfterCellChange(const GameObjectRef& pSubject,
	const int32 oldCellX, const int32 oldCellY, const int32 newCellX, const int32 newCellY)
{
	(void)pSubject; (void)oldCellX; (void)oldCellY; (void)newCellX; (void)newCellY;
}

void GameTickProcessor::processMoveToPositionRequests()
{
	{
		std::lock_guard<Mutex> lock(mMoveToQueueLock);
		mProcessingMoveToInputs.swap(mPendingMoveToInputs);
		mPendingMoveToInputs.clear();
	}

	if (mpNavMeshManager == nullptr || !mpNavMeshManager->IsLoaded())
	{
		if (!mProcessingMoveToInputs.empty())
		{
			NET_ENGINE_LOG_WARN("processMoveToPosition - NavMesh not loaded, skipping {} inputs", mProcessingMoveToInputs.size());
		}
		mProcessingMoveToInputs.clear();
		return;
	}

	// 연속 클릭 시 동일 플레이어의 마지막 요청만 처리 — 중간 요청은 이미 outdated
	{
		std::unordered_map<uint64, size_t> latestIdx;
		for (size_t i = 0; i < mProcessingMoveToInputs.size(); ++i)
		{
			latestIdx[mProcessingMoveToInputs[i].mAccountId] = i;
		}

		if (latestIdx.size() < mProcessingMoveToInputs.size())
		{
			Vector<MoveToPositionEntry> deduped;
			deduped.reserve(latestIdx.size());
			for (const auto& [accountId, idx] : latestIdx)
			{
				deduped.push_back(mProcessingMoveToInputs[idx]);
			}
			mProcessingMoveToInputs = std::move(deduped);
		}
	}

	for (const auto& input : mProcessingMoveToInputs)
	{
		const auto pObject = mpGridManager->GetObjectByAccountId(input.mAccountId);
		if (pObject == nullptr || pObject->GetObjectType() != eGameObjectType::Player)
		{
			continue;
		}

		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);

		// 클라이언트 현재 위치로 서버 위치 동기화 (DB 저장 정확도를 위해)
		if (input.mHasCurrentPosition)
		{
			pPlayer->SetPosition(input.mCurrentX, input.mCurrentY, input.mCurrentZ);
		}

		// 서버 위치가 NavMesh 밖이면 가장 가까운 유효 위치로 snap
		Vector3f start(pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ());
		if (!mpNavMeshManager->IsPositionWalkable(start.mX, start.mY, start.mZ, 200.0f))
		{
			Vector3f snapped;
			if (mpNavMeshManager->GetNearestValidPosition(start.mX, start.mY, start.mZ, 100000.0f, snapped))
			{
				pPlayer->SetPosition(snapped.mX, snapped.mY, snapped.mZ);
				start = snapped;
				NET_ENGINE_LOG_WARN("processMoveToPosition - start off NavMesh, snapped to ({:.1f}, {:.1f}, {:.1f}), accountId: {}",
					snapped.mX, snapped.mY, snapped.mZ, input.mAccountId);
			}
			else
			{
				NET_ENGINE_LOG_ERROR("processMoveToPosition - snap failed! start: ({:.1f}, {:.1f}, {:.1f}), accountId: {}",
					start.mX, start.mY, start.mZ, input.mAccountId);
			}
		}
		const Vector3f end(input.mDestinationX, input.mDestinationY, input.mDestinationZ);

		PathResult result;
		result.mAccountId = input.mAccountId;
		result.mSequenceId = input.mSequenceId;
		result.mMoveSpeed = pPlayer->GetMoveSpeed();

		const bool bPathFound = mpNavMeshManager->FindPath(start, end, result.mWaypoints);
		NET_ENGINE_LOG_INFO("processMoveToPosition - accountId: {}, start: ({:.1f},{:.1f},{:.1f}), end: ({:.1f},{:.1f},{:.1f}), pathFound: {}, waypoints: {}",
			input.mAccountId, start.mX, start.mY, start.mZ, end.mX, end.mY, end.mZ,
			bPathFound, result.mWaypoints.size());
		if (bPathFound)
		{
			pPlayer->SetPath(Vector<Vector3f>(result.mWaypoints));
		}

		mPendingPathResults.push_back(std::move(result));
	}

	mProcessingMoveToInputs.clear();
}


void GameTickProcessor::updatePathFollowing(const float deltaSec)
{
	const auto allObjects = mpGridManager->GetAllObjects();

	for (const auto& pObject : allObjects)
	{
		if (pObject->GetObjectType() != eGameObjectType::Player)
		{
			continue;
		}

		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
		if (!pPlayer->IsFollowingPath())
		{
			continue;
		}

		const Vector3f& waypoint = pPlayer->GetCurrentWaypoint();
		const float dx = waypoint.mX - pPlayer->GetPositionX();
		const float dy = waypoint.mY - pPlayer->GetPositionY();
		const float dz = waypoint.mZ - pPlayer->GetPositionZ();
		const float dist = std::sqrt(dx * dx + dy * dy);

		if (dist < 30.0f)
		{
			pPlayer->SetPosition(waypoint.mX, waypoint.mY, waypoint.mZ);

			if (!pPlayer->AdvanceToNextWaypoint())
			{
				pPlayer->ClearPath();
			}

			continue;
		}

		const float moveDistance = pPlayer->GetMoveSpeed() * deltaSec;
		const float ratio = std::min(moveDistance / dist, 1.0f);

		const float newX = pPlayer->GetPositionX() + dx * ratio;
		const float newY = pPlayer->GetPositionY() + dy * ratio;
		const float newZ = pPlayer->GetPositionZ() + dz * ratio;

		pPlayer->SetPosition(newX, newY, newZ);
		pPlayer->SetVelocity(dx / dist * pPlayer->GetMoveSpeed(), dy / dist * pPlayer->GetMoveSpeed(), 0.0f);
		pPlayer->SetRotationYaw(std::atan2(dy, dx) * 180.0f / 3.14159265f);
	}
}

void GameTickProcessor::sendPathResults()
{
	for (const auto& result : mPendingPathResults)
	{
		const auto pObject = mpGridManager->GetObjectByAccountId(result.mAccountId);
		if (pObject == nullptr || pObject->GetObjectType() != eGameObjectType::Player)
		{
			continue;
		}

		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);

		Protocol::S2C_MOVE_PATH_RES response;
		response.set_sequenceid(result.mSequenceId);
		response.set_movespeed(result.mMoveSpeed);
		response.set_servertimestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());

		for (const auto& wp : result.mWaypoints)
		{
			auto* pWaypoint = response.add_waypoints();
			pWaypoint->set_x(wp.mX);
			pWaypoint->set_y(wp.mY);
			pWaypoint->set_z(wp.mZ);
		}

		sendRelayToClient(pPlayer->GetGatewaySessionId(), pPlayer->GetGatewayServerId(),
			MakeMovementSendBuffer(response, packet_id::S2C_MOVE_PATH_RES));
	}
}
