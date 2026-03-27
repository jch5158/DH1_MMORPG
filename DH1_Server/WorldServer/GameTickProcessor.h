#pragma once
#include <StlTypes.h>
#include "MoveInputEntry.h"
#include "MoveToPositionEntry.h"
#include "NavMeshManager.h"

class GridManager;

class GameTickProcessor
{
public:
	explicit GameTickProcessor(GridManagerRef pGridManager, NavMeshManagerRef pNavMeshManager, const float moveSpeed);
	~GameTickProcessor() = default;

	void ProcessTick(const int64 deltaMs);
	void EnqueueMoveInput(const MoveInputEntry& entry);
	void EnqueueMoveToPosition(const MoveToPositionEntry& entry);

private:
	void processMovementInputs();
	void processMoveToPositionRequests();
	void validateWasdPositions();
	void updatePositions(const float deltaSec);
	void updatePathFollowing(const float deltaSec);
	void sendMoveResults();
	void sendPathResults();
	void processVisibilityChanges();
	void broadcastSnapshots();

	void sendRelayToClient(const uint64 gatewaySessionId, const int32 gatewayServerId, const NetSendBufferRef& pInnerPacket);

	GridManagerRef mpGridManager;
	NavMeshManagerRef mpNavMeshManager;
	float mMoveSpeed;

	Mutex mInputQueueLock;
	Vector<MoveInputEntry> mPendingInputs;

	Mutex mMoveToQueueLock;
	Vector<MoveToPositionEntry> mPendingMoveToInputs;

	// 틱 내 임시 데이터
	Vector<MoveInputEntry> mProcessingInputs;
	Vector<MoveToPositionEntry> mProcessingMoveToInputs;
	Vector<uint64> mMovedActorIds;

	// 경로 결과 (틱 내 전송 대기)
	struct PathResult
	{
		uint64 mAccountId;
		uint32 mSequenceId;
		Vector<Vector3f> mWaypoints;
		float mMoveSpeed;
	};
	Vector<PathResult> mPendingPathResults;
};
