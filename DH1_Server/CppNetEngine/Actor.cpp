#include "pch.h"
#include "Actor.h"
#include "ActorScheduler.h"

IActor::IActor(ActorContext actorContext)
	: mActorContext(std::move(actorContext))
{
}

ActorOverlapped& IActor::GetActorOverlapped()
{
	return mActorContext.mActorOverlapped;
}

void IActor::ClearActorOverlapped()
{
	mActorContext.Clear();
}

ActorSchedulerRef IActor::GetActorSchedulerRef() const
{
	return mActorContext.GetActorSchedulerRef();
}

Actor::Actor(ActorContext actorContext)
	: IActor(std::move(actorContext))
	, mSeed(sSeedBase.fetch_add(1))
	, mbAcquire(false)
	, mJobQueue()
{
}

void Actor::Execute()
{
	JobRef pJob;
	if (mJobQueue.TryDequeue(pJob) == false)
	{
		return;
	}

	if (pJob != nullptr)
	{
		pJob->Execute();
	}
}

bool Actor::TryAcquire()
{
	if (mbAcquire.exchange(true) == true)
	{
		return false;
	}

	return true;
}

void Actor::Release()
{
	mbAcquire.store(false);
}

void Actor::Register()
{
	if (mJobQueue.Count() > 0)
	{
		const ActorSchedulerRef pScheduler = GetActorSchedulerRef();
		if (pScheduler != nullptr)
		{
			pScheduler->Schedule(shared_from_this());
		}
	}
}

void Actor::Flush()
{
	JobRef pJob;
	while (mJobQueue.TryDequeue(pJob))
	{
		pJob->Execute();
	}
}

bool Actor::PushJob(const JobRef& pJob)
{
	return mJobQueue.TryEnqueue(pJob);
}

int32 Actor::GetJobCount()
{
	return mJobQueue.Count();
}

void Actor::Clear()
{
	mJobQueue.Clear();
}

int32 Actor::Count() const
{
	return mJobQueue.Count();
}

int64 Actor::GetSeed() const
{
	return mSeed;
}

std::atomic<int64> Actor::sSeedBase = 0;
