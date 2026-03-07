#include "pch.h"
#include "ActorScheduler.h"
#include "Actor.h"

ActorScheduler::ActorScheduler(std::function<void(const uint32)> pOnHandleError,
	const uint32 timeSliceMs,
	const int32 executeJobCount,
	const int64 tickIntervalMs)
	: mActorIocpHandle(nullptr)
	, mTimeSliceMs(timeSliceMs)
	, mExecuteJobCount(executeJobCount)
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

void ActorScheduler::Schedule(const IActorRef& pActor, const bool bBypassAcquire) const
{
	if (bBypassAcquire || pActor->TryAcquire())
	{
		auto& overlapped = pActor->GetActorOverlapped();
		overlapped.ClearOverlapped();
		overlapped.SetOwner(pActor);

		if (PostQueuedCompletionStatus(mActorIocpHandle, 0, 0, &overlapped) == false)
		{
			overlapped.ClearOverlapped();
			overlapped.ResetOwner();
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

	const int32 gqcsRet = GetQueuedCompletionStatus(mActorIocpHandle, &bytesTransferred, &pCompletionKey, reinterpret_cast<LPOVERLAPPED*>(&pActorOverlapped), mTimeSliceMs);
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
			pActorOverlapped->Clear();

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
