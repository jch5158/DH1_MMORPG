#include "pch.h"
#include "ActorMailbox.h"
#include "Actor.h"
#include "ActorEventEnum.h"

ActorMessageEvent::ActorMessageEvent()
	:IocpEvent(static_cast<uint32>(eActorEventType::ActorMessage))
{
}

ActorMailbox::ActorMailbox()
	: mActorMessageEvent()
	, mMaxExecuteMsgCount(DEFAULT_EXECUTE_MSG_COUNT)
	, mpScheduler(nullptr)
	, mMailbox()
{
}

bool ActorMailbox::Initialize(const IocpObjectRef& pOwner, ActorSchedulerRef scheduler)
{
	if (pOwner == nullptr)
	{
		return false;
	}

	mActorMessageEvent.SetOwner(pOwner);
	mpScheduler = std::move(scheduler);
	return true;
}

ActorMessageEvent& ActorMailbox::GetActorMessageEvent()
{
	return mActorMessageEvent;
}

void ActorMailbox::SetExecuteMsgCount(const int32 executeMsgCount)
{
	mMaxExecuteMsgCount = std::clamp(mMaxExecuteMsgCount, MIN_EXECUTE_MSG_COUNT, MAX_EXECUTE_MSG_COUNT);
}

void ActorMailbox::Post(MessageRef&& pMessage)
{
	(void)mMailbox.TryEnqueue(std::move(pMessage));
	Register();
}

int32 ActorMailbox::GetMessageCount() const
{
	return mMailbox.Count();
}

void ActorMailbox::Process()
{
	const int32 currentMessageCount = mMailbox.Count();
	const int32 executeMessageCount = mMaxExecuteMsgCount < currentMessageCount ? mMaxExecuteMsgCount : currentMessageCount;

	for (int32 i = 0; i < executeMessageCount; ++i)
	{
		MessageRef pMessage;
		if (mMailbox.TryDequeue(pMessage))
		{
			if (pMessage != nullptr)
			{
				pMessage->Execute();
			}
		}
		else
		{
			break;
		}
	}

	Register();
}

void ActorMailbox::Register()
{
	if (mMailbox.Count() > 0 && mpScheduler != nullptr)
	{
		if (mpScheduler->Register(mActorMessageEvent) == false)
		{
			NET_ENGINE_LOG_ERROR("ActorMailbox::Register() - mpScheduler->Register(mActorMessageEvent) failed");
		}
	}
}

void ActorMailbox::Flush()
{
	MessageRef pMessage;
	while (mMailbox.TryDequeue(pMessage))
	{
		if (pMessage != nullptr)
		{
			pMessage->Execute();
		}
	}
}