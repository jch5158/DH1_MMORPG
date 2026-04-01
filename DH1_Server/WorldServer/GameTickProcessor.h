#pragma once
#include <StlTypes.h>
#include "MoveToPositionEntry.h"
#include "NavMeshManager.h"
#include "PointerType.h"

class GridManager;

class GameTickProcessor
{
public:
	explicit GameTickProcessor(GridManagerRef pGridManager, NavMeshManagerRef pNavMeshManager, const float moveSpeed);
	~GameTickProcessor() = default;

	void ProcessTick(const int64 deltaMs);
	void EnqueueMoveToPosition(const MoveToPositionEntry& entry);

	/** S2C_CHAT_NOT 등 채팅용 — 이미 직렬화된 클라이언트 패킷을 게이트웨이 경유 전송 */
	void RelayChatToGatewayClient(uint64 gatewaySessionId, int32 gatewayServerId, const NetSendBufferRef& pInnerPacket);

	/** 스폰 직후: 해당 플레이어에게 주변 엔터티 목록 전송 */
	void NotifySpawnedPlayerVisibleEntities(const PlayerObjectRef& pPlayer);

	/** 엔터티가 셀에 들어왔을 때(비플레이어 이동·다른 플레이어 스폰 등): AOI 내 플레이어들에게 단일 ENTER */
	void NotifyNearbyPlayersAboutEntity(const GameObjectRef& pSubject);

private:
	void processMoveToPositionRequests();
	void updatePathFollowing(const float deltaSec);
	void sendPathResults();
	void processVisibilityChanges();
	void broadcastSnapshots();

	void sendRelayToClient(const uint64 gatewaySessionId, const int32 gatewayServerId, const NetSendBufferRef& pInnerPacket);
	/** 셀 이동 직후: AOI 밖으로 나간 엔티티에 대해 S2C_ENTITY_LEAVE_NOT 전송(클라 잔상 제거) */
	void sendAoiLeavesAfterCellChange(const GameObjectRef& pSubject, int32 oldCellX, int32 oldCellY, int32 newCellX, int32 newCellY);

	GridManagerRef mpGridManager;
	NavMeshManagerRef mpNavMeshManager;
	float mMoveSpeed;

	Mutex mMoveToQueueLock;
	Vector<MoveToPositionEntry> mPendingMoveToInputs;

	// 틱 내 임시 데이터
	Vector<MoveToPositionEntry> mProcessingMoveToInputs;

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
