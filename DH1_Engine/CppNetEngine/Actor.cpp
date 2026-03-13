#include "pch.h"
#include "Actor.h"
#include "ActorScheduler.h"
#include "IocpEvent.h"

//void IActor::Dispatch(class IocpEvent& iocpEvent, const uint32)
//{
//	switch (iocpEvent)
//	{
//	case eIocpEventType::ActorMessage:
//		mMessageQueue.Process();
//		break;
//	default:
//		NET_ENGINE_LOG_ERROR("IActor::Dispatch - iocp event type is unmatched, actorEvent.GetEventType() : {}", static_cast<uint8>(actorEvent.GetEventType()));
//		break;
//	}
//}

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

void Actor::Activate()
{
	mMailbox.SetOwner(shared_from_this());
}

void Actor::Register()
{
	if (TryAcquire())
	{
		mMailbox.Register();
		
		Release();
	}
}

void Actor::Flush()
{
	if (TryAcquire())
	{
		mMailbox.Flush();

		Release();
	}
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
