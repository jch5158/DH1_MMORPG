#include "pch.h"
#include "GridCell.h"
#include "GameObject.h"

GridCell::GridCell(const int32 cellX, const int32 cellY)
	: Actor()
	, mCellX(cellX)
	, mCellY(cellY)
{
}

void GridCell::AddObject(const GameObjectRef& pObject)
{
	mObjects.try_emplace(pObject->GetId(), pObject);
}

void GridCell::RemoveObject(const uint64 actorId)
{
	mObjects.erase(actorId);
}

GameObjectRef GridCell::GetObject(const uint64 actorId) const
{
	const auto iter = mObjects.find(actorId);
	if (iter == mObjects.end())
	{
		return nullptr;
	}

	return iter->second;
}
