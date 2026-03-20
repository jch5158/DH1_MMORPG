#include "pch.h"
#include "ActorManager.h"
#include "Actor.h"

ActorManager::ActorManager()
	: mLock()
	  , mActorMap()
{
}

ActorRef ActorManager::GetActorRef(const uint64 actorId)
{
	SharedLock lock(mLock);

	const auto iter = mActorMap.find(actorId);
	if (iter == mActorMap.end())
	{
		return nullptr;
	}

	return  iter->second;
}

uint64 ActorManager::GetActorCount()
{
	SharedLock lock(mLock);
	return mActorMap.size();
}

bool ActorManager::AddActorRef(ActorRef pActor)
{
	if (pActor == nullptr)
	{
		return false;
	}

	UniqueLock lock(mLock);
	return mActorMap.try_emplace(pActor->GetId(), std::move(pActor)).second;
}

bool ActorManager::RemoveActorRef(const uint64 actorId)
{
	UniqueLock lock(mLock);
	const auto ret = mActorMap.erase(actorId);
	return ret != 0;
}
