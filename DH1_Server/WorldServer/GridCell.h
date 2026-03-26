#pragma once
#include <Actor.h>
#include <StlTypes.h>

class GameObject;

class GridCell final : public Actor
{
public:

	explicit GridCell(const int32 cellX, const int32 cellY);
	virtual ~GridCell() override = default;

	void AddObject(const GameObjectRef& pObject);
	void RemoveObject(const uint64 actorId);
	[[nodiscard]] GameObjectRef GetObject(const uint64 actorId) const;
	[[nodiscard]] const HashMap<uint64, GameObjectRef>& GetAllObjects() const { return mObjects; }
	[[nodiscard]] int32 GetObjectCount() const { return static_cast<int32>(mObjects.size()); }

	[[nodiscard]] int32 GetCellX() const { return mCellX; }
	[[nodiscard]] int32 GetCellY() const { return mCellY; }

private:

	int32 mCellX;
	int32 mCellY;
	HashMap<uint64, GameObjectRef> mObjects;
};
