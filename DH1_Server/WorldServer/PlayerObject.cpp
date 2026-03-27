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

void PlayerObject::UpdateMoveInput(const uint32 sequenceId, const float dirX, const float dirY, const float dirZ, const float rotYaw, const int64 timestamp, const float posZ)
{
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
