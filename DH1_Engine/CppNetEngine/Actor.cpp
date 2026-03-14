#include "pch.h"
#include "Actor.h"
#include "ActorScheduler.h"
#include "IocpEvent.h"

IActor::IActor()
	:mId(sSeedBase.fetch_add(1))
{
}

uint64 IActor::GetId() const
{
	return mId;
}

std::atomic<uint64> IActor::sSeedBase = 0;

Actor::Actor()
	: mbAcquire(false)
	, mMailbox()
{
}

void Actor::Dispatch(class IocpEvent& iocpEvent, const uint32)
{
	switch (iocpEvent.GetEventType())  // NOLINT(clang-diagnostic-switch-enum)
	{
	case eIocpEventType::ActorMessage:
		processActorMessage();
		break;
	default:
		NET_ENGINE_LOG_ERROR("Actor::Dispatch - iocp event type is unmatched, actorEvent.GetEventType() : {}", static_cast<uint8>(iocpEvent.GetEventType()));
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

bool Actor::Activate(ActorScheduler& scheduler)
{
	if (mMailbox.Initialize(shared_from_this(), scheduler) == false)
	{
		return false;
	}

	return true;
}

void Actor::Flush()
{
	mMailbox.Flush();
	Release();
}

IocpEvent& Actor::GetIocpEvent()
{
	return mMailbox.GetActorMessageEvent();
}

int32 Actor::GetMessageCount()
{
	return mMailbox.GetMessageCount();
}

void Actor::Post(MessageRef pMessage)
{
	mMailbox.Post(std::move(pMessage));
}

void Actor::processActorMessage()
{
	if (TryAcquire())
	{
		mMailbox.Process();

		Release(); 
	}
}
