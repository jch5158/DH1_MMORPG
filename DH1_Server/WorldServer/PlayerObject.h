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
	[[nodiscard]] uint32 GetLastSequenceId() const { return mLastSequenceId; }
	[[nodiscard]] int64 GetLastInputTimestamp() const { return mLastInputTimestamp; }

	void SetGatewayInfo(const uint64 sessionId, const int32 serverId);
	void UpdateMoveInput(const uint32 sequenceId, const float dirX, const float dirY, const float dirZ, const float rotYaw, const int64 timestamp, const float posZ);

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
	uint32 mLastSequenceId = 0;
	int64 mLastInputTimestamp = 0;

	// 경로 추적 상태
	Vector<Vector3f> mPath;
	int32 mCurrentWaypointIndex = -1;
	bool mbFollowingPath = false;
};
