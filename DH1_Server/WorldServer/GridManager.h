#pragma once
#include <StlTypes.h>

class GridCell;
class GameObject;
class PlayerObject;

class GridManager
{
public:

	explicit GridManager(const float cellSize, const int32 aoiRange);
	~GridManager() = default;

	// 셀 관리
	[[nodiscard]] GridCellRef GetOrCreateCell(const int32 cellX, const int32 cellY);
	[[nodiscard]] GridCellRef GetCell(const int32 cellX, const int32 cellY) const;
	[[nodiscard]] Vector<GridCellRef> GetCellsInRange(const int32 cellX, const int32 cellY) const;

	// 오브젝트 관리
	void AddObject(const GameObjectRef& pObject, const int32 cellX, const int32 cellY);
	void RemoveObject(const uint64 actorId);
	void MoveObjectToCell(const uint64 actorId, const int32 newCellX, const int32 newCellY);
	[[nodiscard]] GameObjectRef GetObjectByActorId(const uint64 actorId) const;
	[[nodiscard]] GameObjectRef GetObjectByAccountId(const uint64 accountId) const;
	[[nodiscard]] Vector<GameObjectRef> GetAllObjects() const;
	[[nodiscard]] Vector<GameObjectRef> GetObjectsInRange(const int32 cellX, const int32 cellY) const;
	void RemoveObjectsByGateway(const int32 gatewayServerId, Vector<uint64>& outRemovedAccountIds);
	[[nodiscard]] int32 GetObjectCount() const;

	// 좌표 변환
	[[nodiscard]] int32 ComputeCellX(const float positionX) const;
	[[nodiscard]] int32 ComputeCellY(const float positionY) const;
	[[nodiscard]] int32 GetAoiRange() const { return mAoiRange; }

private:

	static int64 makeCellKey(const int32 cellX, const int32 cellY);

	float mCellSize;
	int32 mAoiRange;

	HashMap<int64, GridCellRef> mGridCells;
	HashMap<uint64, GameObjectRef> mObjects;
	HashMap<uint64, uint64> mAccountToActorId;
};
