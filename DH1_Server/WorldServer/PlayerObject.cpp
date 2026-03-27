#include "pch.h"
#include "PlayerObject.h"

PlayerObject::PlayerObject(const uint64 accountId, const uint64 gatewaySessionId, const int32 gatewayServerId)
	: GameObject(eGameObjectType::Player)
	, mAccountId(accountId)
	, mGatewaySessionId(gatewaySessionId)
	, mGatewayServerId(gatewayServerId)
{
}

void PlayerObject::SetGatewayInfo(const uint64 sessionId, const int32 serverId)
{
	mGatewaySessionId = sessionId;
	mGatewayServerId = serverId;
}

void PlayerObject::SetPath(Vector<Vector3f>&& path)
{
	mPath = std::move(path);
	mCurrentWaypointIndex = mPath.empty() ? -1 : 0;
	mbFollowingPath = !mPath.empty();
	mbMoving = mbFollowingPath;
}

void PlayerObject::ClearPath()
{
	mPath.clear();
	mCurrentWaypointIndex = -1;
	mbFollowingPath = false;
	mVelocityX = 0.0f;
	mVelocityY = 0.0f;
	mVelocityZ = 0.0f;
	mbMoving = false;
}

const Vector3f& PlayerObject::GetCurrentWaypoint() const
{
	static const Vector3f zero;
	if (mCurrentWaypointIndex < 0 || mCurrentWaypointIndex >= static_cast<int32>(mPath.size()))
	{
		return zero;
	}
	return mPath[mCurrentWaypointIndex];
}

bool PlayerObject::AdvanceToNextWaypoint()
{
	++mCurrentWaypointIndex;
	return mCurrentWaypointIndex < static_cast<int32>(mPath.size());
}

void PlayerObject::UpdateMoveInput(const uint32 sequenceId, const float dirX, const float dirY, const float dirZ, const float rotYaw, const int64 timestamp, const float posZ)
{
	// WASD 입력 시 경로 추적 중단
	ClearPath();

	const float length = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);

	if (length > 0.001f)
	{
		const float invLength = 1.0f / length;
		mVelocityX = dirX * invLength * mMoveSpeed;
		mVelocityY = dirY * invLength * mMoveSpeed;
		mVelocityZ = dirZ * invLength * mMoveSpeed;
		mbMoving = true;
	}
	else
	{
		mVelocityX = 0.0f;
		mVelocityY = 0.0f;
		mVelocityZ = 0.0f;
		mbMoving = false;
	}

	// 클라이언트 Z 위치 동기화
	mPositionZ = posZ;

	mRotationYaw = rotYaw;
	mLastSequenceId = sequenceId;
	mLastInputTimestamp = timestamp;
}
