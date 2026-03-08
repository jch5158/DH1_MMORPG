#include "pch.h"
#include "ActorScheduler.h"
#include "Actor.h"

ActorScheduler::ActorScheduler(std::function<void(const uint32)> pOnHandleError,
	const uint32 timeSliceMs,
	const int32 maxExecuteJobCount,
	const int64 tickIntervalMs)
	: mActorIocpHandle(nullptr)
	, mTimeSliceMs(timeSliceMs)
	, mMaxExecuteJobCount(maxExecuteJobCount)
	, mTimingWheel(TimingWheel(tickIntervalMs))
	, mpOnHandleError(std::move(pOnHandleError))
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

int32 ActorScheduler::GetMaxExecuteJobCount() const
{
	return mMaxExecuteJobCount;
}

void ActorScheduler::Schedule(ActorEvent& actorEvent) const
{
	if (PostQueuedCompletionStatus(mActorIocpHandle, 0, 0, &actorEvent) == false)
	{
		NET_ENGINE_LOG_ERROR("ActorScheduler::Schedule - PostQueuedCompletionStatus is Failed, errorCode : {}", GetLastError());
	}
}

TimerHandle ActorScheduler::ScheduleDelay(const JobRef& pJob, const IActorRef& pOwner, const uint64 delayMs)
{
	TimerHandle handle = mTimingWheel.AddTiming([pJob, pOwner]() -> void
		{
			JobDispatcher::Post(pJob, pOwner);
		}
	, delayMs);

	return handle;
}

void ActorScheduler::Dispatch()
{
	DWORD bytesTransferred = 0;
	ULONG_PTR pCompletionKey = 0;
	ActorEvent* pActorEvent = nullptr;

	const int32 gqcsRet = GetQueuedCompletionStatus(mActorIocpHandle, &bytesTransferred, &pCompletionKey, reinterpret_cast<LPOVERLAPPED*>(&pActorEvent), mTimeSliceMs);
	if (gqcsRet == 0)
	{
		const uint32 errorCode = GetLastError();
		if (errorCode != WAIT_TIMEOUT)
		{
			mpOnHandleError(errorCode);
		}
	}

	if (pActorEvent != nullptr)
	{
		const IActorRef pActor = pActorEvent->GetOwner();
		if (pActor != nullptr && pActor->TryAcquire())
		{
			pActor->Dispatch(*pActorEvent);
		}
	}

	mTimingWheel.Tick();
}
