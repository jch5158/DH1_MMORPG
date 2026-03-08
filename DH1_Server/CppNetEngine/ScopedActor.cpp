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
	if (!tryAcquireAll())
	{
		Release();
		return false;
	}

	return true;
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

void ScopedActor::SetSpinCount(const int32 spinCount)
{
	mSpinCount = spinCount;
}

int32 ScopedActor::GetSpinCount() const
{
	return mSpinCount;
}

bool ScopedActor::tryAcquireAll()
{
	const int32 spinCount = GetSpinCount();
	const int32 actorSize = static_cast<int32>(mActors.size());

	for (int32 i = 0; i < actorSize; ++i)
	{
		for (int32 curSpin = 0; curSpin < spinCount; ++curSpin)
		{
			if (mActors[i]->TryAcquire())
			{
				mAcquireIndex = i;
				break;
			}

			_mm_pause();
		}

		if (mAcquireIndex != i)
		{
			return false;
		}
	}

	return true;
}
