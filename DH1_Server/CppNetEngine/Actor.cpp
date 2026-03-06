#include "pch.h"
#include "Actor.h"
#include "ActorScheduler.h"

IActor::IActor()
	: mActorOverlapped()
{
}

ActorOverlapped& IActor::GetActorOverlapped()
{
	return mActorOverlapped;
}

void IActor::ClearActorOverlapped()
{
	mActorOverlapped.Clear();
}

Actor::Actor()
	: mSeed(sSeedBase.fetch_add(1))
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

void Actor::Register(const ActorSchedulerRef& pActorScheduler)
{
	if (mJobQueue.Count() > 0)
	{
		pActorScheduler->Schedule(shared_from_this());
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
