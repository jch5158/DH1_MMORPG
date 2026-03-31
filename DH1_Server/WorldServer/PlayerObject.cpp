#include "pch.h"
#include "PlayerObject.h"

#include "Movement.pb.h"

#include <algorithm>
#include <cmath>

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

void PlayerObject::SetCharacterSheet(std::string displayName, const int32 level, const float currentHp, const float maxHp)
{
	mDisplayName = std::move(displayName);
	mLevel = std::max(1, level);
	mMaxHp = std::max(1.f, maxHp);
	mCurrentHp = std::clamp(currentHp, 0.f, mMaxHp);

	NET_ENGINE_LOG_INFO("PlayerObject::SetCharacterSheet accountId={} nameLen={} level={} hp={}/{}",
		mAccountId, mDisplayName.size(), mLevel, mCurrentHp, mMaxHp);
}

void PlayerObject::WriteCharacterSheetTo(Protocol::S2C_SPAWN_POSITION_RES& res) const
{
	std::string name = mDisplayName;
	constexpr size_t kMaxDisplayNameBytes = 256;
	if (name.size() > kMaxDisplayNameBytes)
	{
		name.resize(kMaxDisplayNameBytes);
	}

	const int32 safeLevel = std::clamp(mLevel, 1, 9999);
	float maxHp = mMaxHp;
	if (!std::isfinite(maxHp) || maxHp <= 0.f)
	{
		maxHp = 100.f;
	}
	float curHp = mCurrentHp;
	if (!std::isfinite(curHp))
	{
		curHp = maxHp;
	}
	curHp = std::clamp(curHp, 0.f, maxHp);

	res.set_displayname(std::move(name));
	res.set_level(safeLevel);
	res.set_maxhp(maxHp);
	res.set_currenthp(curHp);
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

