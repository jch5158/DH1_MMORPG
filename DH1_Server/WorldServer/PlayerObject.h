#pragma once
#include "GameObject.h"
#include "NavMeshManager.h"

#include <string>

namespace Protocol
{
	class S2C_SPAWN_POSITION_RES;
}

class PlayerObject final : public GameObject
{
public:

	explicit PlayerObject(const uint64 accountId, const uint64 gatewaySessionId, const int32 gatewayServerId);
	virtual ~PlayerObject() override = default;

	[[nodiscard]] uint64 GetAccountId() const { return mAccountId; }
	[[nodiscard]] uint64 GetGatewaySessionId() const { return mGatewaySessionId; }
	[[nodiscard]] int32 GetGatewayServerId() const { return mGatewayServerId; }

	void SetGatewayInfo(const uint64 sessionId, const int32 serverId);

	void SetCharacterSheet(std::string displayName, int32 level, float currentHp, float maxHp);
	/** S2C_SPAWN_POSITION_RES에 이름·레벨·HP를 넣을 때 공통 검증(UTF-8 길이·레벨·유한 float) */
	void WriteCharacterSheetTo(Protocol::S2C_SPAWN_POSITION_RES& res) const;

	[[nodiscard]] const std::string& GetDisplayName() const { return mDisplayName; }
	[[nodiscard]] int32 GetLevel() const { return mLevel; }
	[[nodiscard]] float GetCurrentHp() const { return mCurrentHp; }
	[[nodiscard]] float GetMaxHp() const { return mMaxHp; }

	// 경로 추적
	void SetPath(Vector<Vector3f>&& path);
	void ClearPath();
	[[nodiscard]] bool IsFollowingPath() const { return mbFollowingPath; }
	[[nodiscard]] const Vector3f& GetCurrentWaypoint() const;
	[[nodiscard]] bool AdvanceToNextWaypoint();

private:

	uint64 mAccountId;
	uint64 mGatewaySessionId;
	int32 mGatewayServerId;

	std::string mDisplayName;
	int32 mLevel = 1;
	float mCurrentHp = 100.f;
	float mMaxHp = 100.f;

	// 경로 추적 상태
	Vector<Vector3f> mPath;
	int32 mCurrentWaypointIndex = -1;
	bool mbFollowingPath = false;
};
