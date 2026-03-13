#include "pch.h"
#include "ActorScheduler.h"
#include "Actor.h"

ActorScheduler::ActorScheduler(std::function<void(const uint32)> onHandleError,
	const uint32 timeSliceMs,
	const int32 maxExecuteMessageCount,
	const int64 tickIntervalMs)
	: mActorIocpHandle(nullptr)
	, mTimeSliceMs(timeSliceMs)
	, mMaxExecuteMessageCount(maxExecuteMessageCount)
	, mTimingWheel(TimingWheel(tickIntervalMs))
	, mOnHandleError(std::move(onHandleError))
{
	mActorIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (mActorIocpHandle == nullptr)
	{
		CrashReporter::Crash();
	}
}

ActorScheduler::~ActorScheduler()
{
	if (mActorIocpHandle != nullptr)
	{
		CloseHandle(mActorIocpHandle);
	}
}

int32 ActorScheduler::GetMaxExecuteMessageCount() const
{
	return mMaxExecuteMessageCount;
}

bool ActorScheduler::Register(const IocpObjectRef& pIocpObject)
{
	const IActorRef pActor = std::static_pointer_cast<IActor>(pIocpObject);
	if (pActor == nullptr)
	{
		return false;
	}

	if (PostQueuedCompletionStatus(mActorIocpHandle, 0, 0, &pActor->GetIocpEvent()) == false)
	{
		NET_ENGINE_LOG_ERROR("ActorScheduler::Register - PostQueuedCompletionStatus is Failed, errorCode : {}", GetLastError());
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

	const int32 gqcsRet = GetQueuedCompletionStatus(mActorIocpHandle, &bytesTransferred, &pCompletionKey, reinterpret_cast<LPOVERLAPPED*>(&pActorMessageEvent), mTimeSliceMs);
	if (gqcsRet == 0)
	{
		const uint32 errorCode = GetLastError();
		if (errorCode != WAIT_TIMEOUT)
		{
			mOnHandleError(errorCode);
		}
	}

	if (pActorMessageEvent != nullptr)
	{
		const IActorRef pActor = std::static_pointer_cast<IActor>(pActorMessageEvent->GetOwner());
		if (pActor != nullptr && pActor->TryAcquire())
		{
			pActor->Dispatch(*pActorMessageEvent, 0);
		}
	}

	mTimingWheel.Tick();
}
