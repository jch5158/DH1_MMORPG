#include "pch.h"
#include "ActorScheduler.h"
#include "Actor.h"

ActorScheduler::ActorScheduler(std::function<void(const uint32)> pOnHandleError,
	const uint32 timeSliceMs,
	const int32 executeJobCount,
	const int64 tickIntervalMs)
	: mJobIocpHandle(nullptr)
	, mTimeSliceMs(timeSliceMs)
	, mExecuteJobCount(executeJobCount)
	, mTimingWheel(TimingWheel(tickIntervalMs))
	, mpOnHandleError(std::move(pOnHandleError))
{
	mJobIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (mJobIocpHandle == nullptr)
	{
		CrashReporter::Crash();
	}
}

ActorScheduler::~ActorScheduler()
{
	if (mJobIocpHandle != nullptr)
	{
		CloseHandle(mJobIocpHandle);
	}
}

void ActorScheduler::Schedule(const IActorRef& pActor, const bool bBypassAcquire) const
{
	auto& overlapped = pActor->GetActorOverlapped();
	overlapped.ClearOverlapped();
	overlapped.SetOwner(pActor);

	if (bBypassAcquire || pActor->TryAcquire())
	{
		if (PostQueuedCompletionStatus(mJobIocpHandle, 0, 0, &overlapped) == false)
		{
			NET_ENGINE_LOG_ERROR("ActorScheduler::Push - PostQueuedCompletionStatus is Failed, errorCode : {}", GetLastError());
		}

		if (!bBypassAcquire)
		{
			pActor->Release();
		}
	}
}

TimerHandle ActorScheduler::ScheduleDelay(JobRef pJob, IActorRef pOwner, const uint64 delayMs)
{
	TimerHandle handle = mTimingWheel.AddTiming([pCaptureJob = std::move(pJob), pCaptureOwner = std::move(pOwner), pScheduler = shared_from_this()]() mutable -> void
		{
			JobDispatcher::Post(pCaptureJob, pCaptureOwner);
		}
	, delayMs);

	return handle;
}

void ActorScheduler::Dispatch()
{
	DWORD bytesTransferred = 0;
	ULONG_PTR pCompletionKey = 0;
	ActorOverlapped* pActorOverlapped = nullptr;

	const int32 gqcsRet = GetQueuedCompletionStatus(mJobIocpHandle, &bytesTransferred, &pCompletionKey, reinterpret_cast<LPOVERLAPPED*>(&pActorOverlapped), mTimeSliceMs);
	if (gqcsRet == 0)
	{
		const uint32 errorCode = GetLastError();
		if (errorCode != WAIT_TIMEOUT)
		{
			mpOnHandleError(errorCode);
		}
	}

	if (pActorOverlapped != nullptr)
	{
		const IActorRef pActor = pActorOverlapped->GetOwner();
		if (pActor != nullptr && pActor->TryAcquire())
		{
			const int32 currentJobCount = pActor->GetJobCount();
			const int32 executeJobCount = mExecuteJobCount < currentJobCount ? mExecuteJobCount : currentJobCount;

			for (int32 i = 0; i < executeJobCount; ++i)
			{
				pActor->Execute();
			}

			pActor->ClearActorOverlapped();

			pActor->Release();

			pActor->Register();
		}
	}

	mTimingWheel.Tick();
}
