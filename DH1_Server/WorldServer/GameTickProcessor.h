#pragma once
#include <StlTypes.h>
#include "MoveInputEntry.h"

class GridManager;

class GameTickProcessor
{
public:
	explicit GameTickProcessor(GridManagerRef pGridManager, const float moveSpeed);
	~GameTickProcessor() = default;

	void ProcessTick(const int64 deltaMs);
	void EnqueueMoveInput(const MoveInputEntry& entry);

private:
	void processMovementInputs();
	void updatePositions(const float deltaSec);
	void processVisibilityChanges();
	void broadcastSnapshots();
	void sendMoveResults();

	void sendRelayToClient(const uint64 gatewaySessionId, const int32 gatewayServerId, const NetSendBufferRef& pInnerPacket);

	GridManagerRef mpGridManager;
	float mMoveSpeed;

	Mutex mInputQueueLock;
	Vector<MoveInputEntry> mPendingInputs;

	// 틱 내 임시 데이터
	Vector<MoveInputEntry> mProcessingInputs;
	Vector<uint64> mMovedActorIds;
};
