#include "pch.h"
#include "ActorLoadBalancer.h"

ActorLoadBalancer::ActorLoadBalancer(const Vector<uint64>& actorIds)
	: mActorIds(actorIds)
	, mActorCount(static_cast<uint32>(actorIds.size()))
{
	mLoadCounts = std::make_unique<std::atomic<int32>[]>(mActorCount);
	for (uint32 i = 0; i < mActorCount; ++i)
	{
		mLoadCounts[i].store(0, std::memory_order_relaxed);
	}
}

uint64 ActorLoadBalancer::AssignOrGet(const uint64 routingKey)
{
	// 이미 배정된 경우 빠르게 반환 (read lock)
	{
		SharedLock readLock(mRoutingLock);
		const auto iter = mRoutingMap.find(routingKey);
		if (iter != mRoutingMap.end())
		{
			return mActorIds[iter->second];
		}
	}

	// 신규 배정 — 가장 부하가 적은 Actor 선택 (write lock)
	UniqueLock writeLock(mRoutingLock);

	// 이중 체크 (다른 스레드가 이미 배정했을 수 있음)
	const auto iter = mRoutingMap.find(routingKey);
	if (iter != mRoutingMap.end())
	{
		return mActorIds[iter->second];
	}

	// 최소 부하 Actor 탐색
	uint32 minIndex = 0;
	int32 minLoad = mLoadCounts[0].load(std::memory_order_relaxed);
	for (uint32 i = 1; i < mActorCount; ++i)
	{
		const int32 load = mLoadCounts[i].load(std::memory_order_relaxed);
		if (load < minLoad)
		{
			minLoad = load;
			minIndex = i;
		}
	}

	mRoutingMap[routingKey] = minIndex;
	mLoadCounts[minIndex].fetch_add(1, std::memory_order_relaxed);

	return mActorIds[minIndex];
}

void ActorLoadBalancer::Release(const uint64 routingKey)
{
	UniqueLock writeLock(mRoutingLock);

	const auto iter = mRoutingMap.find(routingKey);
	if (iter == mRoutingMap.end())
	{
		return;
	}

	const uint32 actorIndex = iter->second;
	mRoutingMap.erase(iter);
	mLoadCounts[actorIndex].fetch_sub(1, std::memory_order_relaxed);
}

uint64 ActorLoadBalancer::GetNextRoundRobin()
{
	const uint64 index = mRoundRobinIndex.fetch_add(1, std::memory_order_relaxed) % mActorIds.size();
	return mActorIds[index];
}
