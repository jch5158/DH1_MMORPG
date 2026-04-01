#include "pch.h"
#include "GameObject.h"

GameObject::GameObject(const eGameObjectType objectType)
	: Actor()
	, mObjectType(objectType)
{
}

void GameObject::SetPosition(const float x, const float y, const float z)
{
	mPositionX = x;
	mPositionY = y;
	mPositionZ = z;
}

void GameObject::SetVelocity(const float x, const float y, const float z)
{
	mVelocityX = x;
	mVelocityY = y;
	mVelocityZ = z;
}

void GameObject::SetOccupiedGridCoords(const int32 cellX, const int32 cellY)
{
	mbRegisteredInGrid = true;
	mOccupiedCellX = cellX;
	mOccupiedCellY = cellY;
}

void GameObject::ClearOccupiedGridCoords()
{
	mbRegisteredInGrid = false;
}
