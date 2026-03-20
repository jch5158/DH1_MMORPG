#include "pch.h"
#include "ActorScheduler.h"
#include "Actor.h"
#include "SocketUtils.h"

ActorScheduler::ActorScheduler(const ActorSchedulerConfig& config)
	: IocpCore(config.runningThreadCount)
	, mWaitTimeoutMs(config.waitTimeoutMs)
	, mTimingWheel(config.tickIntervalMs)
	, mOnHandleError(config.onHandleError)
{
}

bool ActorScheduler::Register(const IocpObjectRef& pIocpObject)
{
	const IActorRef pActor = std::static_pointer_cast<IActor>(pIocpObject);
	if (pActor == nullptr)
	{
		return false;
	}

	if (PostQueuedCompletionStatus(mIocpHandle, 0, 0, &pActor->GetIocpEvent()) == false)
	{
		return false;
	}

	return true;
}

TimerHandle ActorScheduler::RegisterDelay(std::function<void()> delayFunction, const uint64 delayMs)
{
	TimerHandle handle = mTimingWheel.AddTiming([capFunction = std::move(delayFunction)]()->void
		{
			capFunction();
		}, delayMs);

	return handle;
}

void ActorScheduler::Dispatch()
{
	DWORD bytesTransferred = 0;
	ULONG_PTR pCompletionKey = 0;
	ActorMessageEvent* pActorMessageEvent = nullptr;

	const int32 gqcsRet = GetQueuedCompletionStatus(mIocpHandle, &bytesTransferred, &pCompletionKey, reinterpret_cast<LPOVERLAPPED*>(&pActorMessageEvent), mWaitTimeoutMs);
	if (gqcsRet == 0)
	{
		const uint32 errorCode = GetLastError();
		if (!SocketUtils::IsLoggingIgnorableIocpError(errorCode))
		{
			if (mOnHandleError != nullptr)
			{
				mOnHandleError(errorCode);
			}
		}
	}

	if (pActorMessageEvent != nullptr)
	{
		const IActorRef pActor = std::static_pointer_cast<IActor>(pActorMessageEvent->GetOwner());
		if (pActor != nullptr)
		{
			pActor->Dispatch(*pActorMessageEvent, 0);
		}
	}

	mTimingWheel.Tick();
}
