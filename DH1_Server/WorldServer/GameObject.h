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

	// AOI
	[[nodiscard]] uint64 GetGridCellId() const { return mGridCellId; }
	void SetGridCellId(const uint64 cellId) { mGridCellId = cellId; }

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
};
