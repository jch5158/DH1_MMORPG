#include "pch.h"
#include "GridManager.h"
#include "GridCell.h"
#include "GameObject.h"
#include "PlayerObject.h"

GridManager::GridManager(const float cellSize, const int32 aoiRange)
	: mCellSize(cellSize)
	, mAoiRange(aoiRange)
{
}

GridCellRef GridManager::GetOrCreateCell(const int32 cellX, const int32 cellY)
{
	const int64 key = makeCellKey(cellX, cellY);
	const auto iter = mGridCells.find(key);
	if (iter != mGridCells.end())
	{
		return iter->second;
	}

	auto pCell = cpp_net_engine::MakeShared<GridCell>(cellX, cellY);
	mGridCells.emplace(key, pCell);
	return pCell;
}

GridCellRef GridManager::GetCell(const int32 cellX, const int32 cellY) const
{
	const int64 key = makeCellKey(cellX, cellY);
	const auto iter = mGridCells.find(key);
	if (iter == mGridCells.end())
	{
		return nullptr;
	}

	return iter->second;
}

Vector<GridCellRef> GridManager::GetCellsInRange(const int32 cellX, const int32 cellY) const
{
	Vector<GridCellRef> result;

	for (int32 dx = -mAoiRange; dx <= mAoiRange; ++dx)
	{
		for (int32 dy = -mAoiRange; dy <= mAoiRange; ++dy)
		{
			const auto pCell = GetCell(cellX + dx, cellY + dy);
			if (pCell != nullptr)
			{
				result.push_back(pCell);
			}
		}
	}

	return result;
}

void GridManager::AddObject(const GameObjectRef& pObject, const int32 cellX, const int32 cellY)
{
	const uint64 actorId = pObject->GetId();

	auto pCell = GetOrCreateCell(cellX, cellY);
	pCell->AddObject(pObject);
	pObject->SetGridCellId(pCell->GetId());

	mObjects.emplace(actorId, pObject);

	// PlayerObject인 경우 accountId 역참조 등록
	if (pObject->GetObjectType() == eGameObjectType::Player)
	{
		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
		mAccountToActorId.emplace(pPlayer->GetAccountId(), actorId);
	}

	NET_ENGINE_LOG_INFO("GridManager::AddObject - actorId: {}, cell: ({}, {})", actorId, cellX, cellY);
}

void GridManager::RemoveObject(const uint64 actorId)
{
	const auto iter = mObjects.find(actorId);
	if (iter == mObjects.end())
	{
		return;
	}

	const auto& pObject = iter->second;
	const uint64 gridCellId = pObject->GetGridCellId();

	// 셀에서 제거
	for (auto& [key, pCell] : mGridCells)
	{
		if (pCell->GetId() == gridCellId)
		{
			pCell->RemoveObject(actorId);
			break;
		}
	}

	// PlayerObject인 경우 accountId 역참조 제거
	if (pObject->GetObjectType() == eGameObjectType::Player)
	{
		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
		mAccountToActorId.erase(pPlayer->GetAccountId());
	}

	mObjects.erase(iter);

	NET_ENGINE_LOG_INFO("GridManager::RemoveObject - actorId: {}", actorId);
}

void GridManager::MoveObjectToCell(const uint64 actorId, const int32 newCellX, const int32 newCellY)
{
	const auto iter = mObjects.find(actorId);
	if (iter == mObjects.end())
	{
		return;
	}

	const auto& pObject = iter->second;
	const uint64 oldCellId = pObject->GetGridCellId();

	// 이전 셀에서 제거
	for (auto& [key, pCell] : mGridCells)
	{
		if (pCell->GetId() == oldCellId)
		{
			pCell->RemoveObject(actorId);
			break;
		}
	}

	// 새 셀에 추가
	auto pNewCell = GetOrCreateCell(newCellX, newCellY);
	pNewCell->AddObject(pObject);
	pObject->SetGridCellId(pNewCell->GetId());

	NET_ENGINE_LOG_INFO("GridManager::MoveObjectToCell - actorId: {}, newCell: ({}, {})", actorId, newCellX, newCellY);
}

GameObjectRef GridManager::GetObjectByActorId(const uint64 actorId) const
{
	const auto iter = mObjects.find(actorId);
	if (iter == mObjects.end())
	{
		return nullptr;
	}

	return iter->second;
}

GameObjectRef GridManager::GetObjectByAccountId(const uint64 accountId) const
{
	const auto accountIter = mAccountToActorId.find(accountId);
	if (accountIter == mAccountToActorId.end())
	{
		return nullptr;
	}

	return GetObjectByActorId(accountIter->second);
}

Vector<GameObjectRef> GridManager::GetAllObjects() const
{
	Vector<GameObjectRef> result;
	result.reserve(mObjects.size());

	for (const auto& [actorId, pObject] : mObjects)
	{
		result.push_back(pObject);
	}

	return result;
}

Vector<GameObjectRef> GridManager::GetObjectsInRange(const int32 cellX, const int32 cellY) const
{
	Vector<GameObjectRef> result;

	const auto cells = GetCellsInRange(cellX, cellY);
	for (const auto& pCell : cells)
	{
		for (const auto& [actorId, pObject] : pCell->GetAllObjects())
		{
			result.push_back(pObject);
		}
	}

	return result;
}

void GridManager::RemoveObjectsByGateway(const int32 gatewayServerId, Vector<uint64>& outRemovedAccountIds)
{
	Vector<uint64> actorIdsToRemove;

	for (const auto& [actorId, pObject] : mObjects)
	{
		if (pObject->GetObjectType() != eGameObjectType::Player)
		{
			continue;
		}

		const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
		if (pPlayer->GetGatewayServerId() == gatewayServerId)
		{
			actorIdsToRemove.push_back(actorId);
			outRemovedAccountIds.push_back(pPlayer->GetAccountId());
		}
	}

	for (const uint64 actorId : actorIdsToRemove)
	{
		RemoveObject(actorId);
	}
}

int32 GridManager::GetObjectCount() const
{
	return static_cast<int32>(mObjects.size());
}

int32 GridManager::ComputeCellX(const float positionX) const
{
	return static_cast<int32>(std::floor(positionX / mCellSize));
}

int32 GridManager::ComputeCellY(const float positionY) const
{
	return static_cast<int32>(std::floor(positionY / mCellSize));
}

int64 GridManager::makeCellKey(const int32 cellX, const int32 cellY)
{
	return (static_cast<int64>(cellX) << 32) | static_cast<int64>(static_cast<uint32>(cellY));
}
