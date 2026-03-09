#include "pch.h"
#include "Actor.h"
#include "ActorScheduler.h"
#include "ActorEvent.h"

IActor::IActor(ActorSchedulerRef pScheduler)
	: mpScheduler(std::move(pScheduler))
	, mJobQueue()
{
}

void IActor::Activate()
{
	mJobQueue.Initialize(shared_from_this(), mpScheduler);
}

void IActor::Register()
{
	mJobQueue.Register();
}

void IActor::Flush()
{
	mJobQueue.Flush();
}

bool IActor::PushJob(JobRef pJob)
{
	return mJobQueue.PushJob(std::move(pJob));
}

int32 IActor::GetJobCount() const
{
	return mJobQueue.GetJobCount();
}

ActorSchedulerRef IActor::GetActorSchedulerRef() const
{
	return mpScheduler;
}

Actor::Actor(ActorSchedulerRef pScheduler)
	: IActor(std::move(pScheduler))
	, mSeed(sSeedBase.fetch_add(1))
	, mbAcquire(false)
{
}

void IActor::Dispatch(ActorEvent& actorEvent)
{
	switch (actorEvent.GetEventType())
	{
	case eActorEventType::Job:
		mJobQueue.Process();
		break;
	default:  // NOLINT(clang-diagnostic-covered-switch-default)
		NET_ENGINE_LOG_ERROR("IActor::Dispatch - iocp event type is unmatched, actorEvent.GetEventType() : {}", static_cast<uint8>(actorEvent.GetEventType()));
		break;
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

int64 Actor::GetSeed() const
{
	return mSeed;
}

std::atomic<int64> Actor::sSeedBase = 0;
