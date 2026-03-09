#include "pch.h"
#include "ScopedActor.h"

void ScopedActor::Dispatch(ActorEvent& actorEvent)
{
	switch (actorEvent.GetEventType())
	{
	case eActorEventType::Job:
		mJobQueue.Process();
		break;
	default:  // NOLINT(clang-diagnostic-covered-switch-default)
		NET_ENGINE_LOG_ERROR("ScopedActor::Dispatch - iocp event type is unmatched, actorEvent.GetEventType() : {}", static_cast<uint8>(actorEvent.GetEventType()));
		break;
	}
}

bool ScopedActor::TryAcquire()
{
	bool bSuccess = false;
	const int32 retryLimit = GetRetryLimit();

	for (int32 retry = 0; retry < retryLimit; ++retry)
	{
		if (tryAcquireAll())
		{
			bSuccess = true;
			break;
		}

		Release();

		if (retry < 10)
		{
			_mm_pause();
		}
		else if (retry < 50)
		{
			std::this_thread::yield();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	if (!bSuccess)
	{
		Release();
	}

	return bSuccess;
}

void ScopedActor::Release()
{
	for (int32 i = mAcquireIndex; i >= 0; --i)
	{
		mActors[i]->Release();
	}

	mAcquireIndex = -1;
}

void ScopedActor::Post(CallbackType&& callback)
{
	const auto pJob = cpp_net_engine::MakeShared<Job>(std::move(callback));
	JobDispatcher::Post(pJob, shared_from_this());
}

TimerHandle ScopedActor::PostDelay(CallbackType&& callback, const int64 delayMs)
{
	const auto pJob = cpp_net_engine::MakeShared<Job>(std::move(callback));
	TimerHandle handle = JobDispatcher::PostDelay(pJob, shared_from_this(), GetActorSchedulerRef(), delayMs);
	return handle;
}

void ScopedActor::SetRetryLimit(const int32 retryLimit)
{
	mRetryLimit = retryLimit;
}

int32 ScopedActor::GetRetryLimit() const
{
	return mRetryLimit;
}

void ScopedActor::SetSpinLimit(const int32 spinCount)
{
	mSpinLimit = spinCount;
}

int32 ScopedActor::GetSpinLimit() const
{
	return mSpinLimit;
}

bool ScopedActor::tryAcquireAll()
{
	bool bAllAcquired = true; // 최종 결과를 담을 변수
	const int32 spinCount = GetSpinLimit();
	const int32 actorSize = static_cast<int32>(mActors.size());

	for (int32 i = 0; i < actorSize; ++i)
	{
		bool bAcquired = false;
		const int32 spinLimit = (i == 0) ? spinCount : 1;

		for (int32 curSpin = 0; curSpin < spinLimit; ++curSpin)
		{
			if (mActors[i]->TryAcquire())
			{
				mAcquireIndex = i;
				bAcquired = true;
				break;
			}

			_mm_pause(); // 하이퍼스레딩 최적화
		}

		if (!bAcquired)
		{
			bAllAcquired = false;
			break;
		}
	}

	return bAllAcquired;
}
