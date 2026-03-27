#pragma once

// Actor 풀에 대한 부하 기반 라우팅
// - AssignOrGet: 같은 routingKey는 항상 같은 Actor로 (유저별 순서 보장)
// - Release: 배정 해제 시 카운트 감소
// - GetNextRoundRobin: 유저 무관한 범용 라운드 로빈
class ActorLoadBalancer final
{
public:

	explicit ActorLoadBalancer(const Vector<uint64>& actorIds);
	~ActorLoadBalancer() = default;

	ActorLoadBalancer(const ActorLoadBalancer&) = delete;
	ActorLoadBalancer& operator=(const ActorLoadBalancer&) = delete;
	ActorLoadBalancer(ActorLoadBalancer&&) = delete;
	ActorLoadBalancer& operator=(ActorLoadBalancer&&) = delete;

	// routingKey에 대해 이미 배정된 Actor가 있으면 반환, 없으면 최소 부하 Actor에 배정
	[[nodiscard]] uint64 AssignOrGet(uint64 routingKey);

	// routingKey 배정 해제 (유저 로그아웃 등)
	void Release(uint64 routingKey);

	// 라운드 로빈 (범용)
	[[nodiscard]] uint64 GetNextRoundRobin();

private:

	Vector<uint64> mActorIds;
	std::unique_ptr<std::atomic<int32>[]> mLoadCounts;
	uint32 mActorCount = 0;
	std::atomic<uint64> mRoundRobinIndex{0};

	SharedMutex mRoutingLock;
	HashMap<uint64, uint32> mRoutingMap; // routingKey → actor index
};
