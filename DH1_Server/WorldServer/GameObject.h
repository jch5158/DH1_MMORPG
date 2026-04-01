#pragma once
#include <Actor.h>
#include "GameObjectType.h"

class GameObject : public Actor
{
public:

	explicit GameObject(const eGameObjectType objectType);
	virtual ~GameObject() override = default;

	[[nodiscard]] eGameObjectType GetObjectType() const { return mObjectType; }

	// Transform
	[[nodiscard]] float GetPositionX() const { return mPositionX; }
	[[nodiscard]] float GetPositionY() const { return mPositionY; }
	[[nodiscard]] float GetPositionZ() const { return mPositionZ; }
	[[nodiscard]] float GetVelocityX() const { return mVelocityX; }
	[[nodiscard]] float GetVelocityY() const { return mVelocityY; }
	[[nodiscard]] float GetVelocityZ() const { return mVelocityZ; }
	[[nodiscard]] float GetRotationYaw() const { return mRotationYaw; }
	[[nodiscard]] float GetMoveSpeed() const { return mMoveSpeed; }
	[[nodiscard]] bool IsMoving() const { return mbMoving; }

	void SetPosition(const float x, const float y, const float z);
	void SetVelocity(const float x, const float y, const float z);
	void SetRotationYaw(const float yaw) { mRotationYaw = yaw; }
	void SetMoveSpeed(const float speed) { mMoveSpeed = speed; }
	void SetMoving(const bool moving) { mbMoving = moving; }

	// AOI — GridCell::Actor id (디버그·기존 코드) + 논리 셀 좌표 (Remove/Move 시 O(1) 조회)
	[[nodiscard]] uint64 GetGridCellId() const { return mGridCellId; }
	void SetGridCellId(const uint64 cellId) { mGridCellId = cellId; }

	[[nodiscard]] bool IsRegisteredInGrid() const { return mbRegisteredInGrid; }
	[[nodiscard]] int32 GetOccupiedCellX() const { return mOccupiedCellX; }
	[[nodiscard]] int32 GetOccupiedCellY() const { return mOccupiedCellY; }
	void SetOccupiedGridCoords(const int32 cellX, const int32 cellY);
	void ClearOccupiedGridCoords();

protected:

	eGameObjectType mObjectType;

	float mPositionX = 0.0f;
	float mPositionY = 0.0f;
	float mPositionZ = 0.0f;
	float mVelocityX = 0.0f;
	float mVelocityY = 0.0f;
	float mVelocityZ = 0.0f;
	float mRotationYaw = 0.0f;

	float mMoveSpeed = 600.0f;
	bool mbMoving = false;

	uint64 mGridCellId = 0;

	bool mbRegisteredInGrid = false;
	int32 mOccupiedCellX = 0;
	int32 mOccupiedCellY = 0;
};
