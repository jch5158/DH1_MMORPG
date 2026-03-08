#include "pch.h"
#include "ActorEvent.h"


ActorEvent::ActorEvent(const eActorEventType eventType)
	: OVERLAPPED{}
	, mEventType(eventType)
	, mpOwnerWeak()
{
}

IActorRef ActorEvent::GetOwner() const
{
	IActorRef pOwner = mpOwnerWeak.lock();

	return pOwner;
}

void ActorEvent::SetOwner(const IActorRef& pOwner)
{
	if (pOwner == nullptr)
	{
		return;
	}

	mpOwnerWeak = pOwner;
}

eActorEventType ActorEvent::GetEventType() const
{
	return mEventType;
}

JobActorEvent::JobActorEvent()
	:ActorEvent(eActorEventType::Job)
{
}
