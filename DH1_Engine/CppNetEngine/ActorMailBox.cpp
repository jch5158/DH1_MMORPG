#include "pch.h"
#include "ActorMailBox.h"
#include "Actor.h"

ActorMessageEvent::ActorMessageEvent()
	:IocpEvent(eIocpEventType::ActorMessage)
{}

ActorMailBox::ActorMailBox(const int32 executeMsgCount)
	: mActorMessageEvent()
	, mMaxExecuteMsgCount(executeMsgCount)
	, mMailbox()
{
}

void ActorMailBox::SetOwner(const IocpObjectRef& pOwner)
{
	if (pOwner == nullptr)
	{
		return;
	}

	mActorMessageEvent.SetOwner(pOwner);
}

ActorMessageEvent& ActorMailBox::GetActorMessageEvent()
{
	return mActorMessageEvent;
}

void ActorMailBox::Post(MessageRef&& pMessage)
{
	(void)mMailbox.TryEnqueue(std::move(pMessage));
}

int32 ActorMailBox::GetMessageCount() const
{
	return mMailbox.Count();
}

void ActorMailBox::Process()
{
	const IActorRef pActor = std::static_pointer_cast<IActor>(mActorMessageEvent.GetOwner());
	if (pActor != nullptr)
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
	}
}

void ActorMailBox::Register()
{
	const IActorRef pActor = std::static_pointer_cast<IActor>(mActorMessageEvent.GetOwner());
	if (pActor != nullptr)
	{
		if (mMailbox.Count() > 0)
		{
			// TODO : 
			//if (mpScheduler != nullptr)
			//{
			//	mpScheduler->Schedule(mMessageActorEvent);
			//}
		}
	}
}

void ActorMailBox::Flush()
{
	const IActorRef pActor = std::static_pointer_cast<IActor>(mActorMessageEvent.GetOwner());
	if (pActor != nullptr && pActor->TryAcquire())
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
}