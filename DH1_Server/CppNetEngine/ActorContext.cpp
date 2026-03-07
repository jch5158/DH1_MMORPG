#include "pch.h"
#include "ActorContext.h"

ActorContext::ActorContext(const ActorSchedulerRef& pScheduler)
	: mActorOverlapped()
	, mpSchedulerWeak(pScheduler)
{
}

ActorContext::~ActorContext()
{
	mActorOverlapped.Clear();
	mpSchedulerWeak.reset();
}

IActorRef ActorContext::GetOwner()
{
	return mActorOverlapped.GetOwner();
}

void ActorContext::SetOwner(const IActorRef& pOwner)
{
	return mActorOverlapped.SetOwner(pOwner);
}

void ActorContext::ResetOwner()
{
	mActorOverlapped.ResetOwner();
}

void ActorContext::Clear()
{
	mActorOverlapped.Clear();
}

void ActorContext::ClearOverlapped()
{
	mActorOverlapped.ClearOverlapped();
}

void ActorContext::SetActorScheduler(const ActorSchedulerRef& pScheduler)
{
	if (pScheduler == nullptr)
	{
		return;
	}

	mpSchedulerWeak = pScheduler;
}

void ActorContext::ResetActorSchedulerRef()
{
	mpSchedulerWeak.reset();
}

ActorSchedulerRef ActorContext::GetActorSchedulerRef() const
{
	return mpSchedulerWeak.lock();
}
