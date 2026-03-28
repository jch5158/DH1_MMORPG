#pragma once
#include "GameObject.h"
#include "NavMeshManager.h"

class PlayerObject final : public GameObject
{
public:

	explicit PlayerObject(const uint64 accountId, const uint64 gatewaySessionId, const int32 gatewayServerId);
	virtual ~PlayerObject() override = default;

	[[nodiscard]] uint64 GetAccountId() const { return mAccountId; }
	[[nodiscard]] uint64 GetGatewaySessionId() const { return mGatewaySessionId; }
	[[nodiscard]] int32 GetGatewayServerId() const { return mGatewayServerId; }

	void SetGatewayInfo(const uint64 sessionId, const int32 serverId);

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

	// 경로 추적 상태
	Vector<Vector3f> mPath;
	int32 mCurrentWaypointIndex = -1;
	bool mbFollowingPath = false;
};
