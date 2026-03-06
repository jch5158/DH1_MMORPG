#include "pch.h"
#include "ScopedActor.h"

void ScopedActor::Execute()
{

	JobRef pJob;
	if (mJobQueue.TryDequeue(pJob) == false)
	{
		return;
	}

	if (pJob != nullptr)
	{
		pJob->Execute();
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

void ScopedActor::Register(const ActorSchedulerRef& pActorScheduler)
{
	if (mJobQueue.Count() > 0)
	{
		pActorScheduler->Schedule(shared_from_this(), true);
	}
}

void ScopedActor::Flush()
{
	JobRef pJob;
	while (mJobQueue.TryDequeue(pJob))
	{
		pJob->Execute();
	}
}

bool ScopedActor::PushJob(const JobRef& pJob)
{
	return mJobQueue.TryEnqueue(pJob);
}

int32 ScopedActor::GetJobCount()
{
	return mJobQueue.Count();
}

void ScopedActor::Post(const ActorSchedulerRef& pScheduler, CallbackType&& callback)
{
	const auto pJob = cpp_net_engine::MakeShared<Job>(std::move(callback));
	JobDispatcher::Post(pJob, shared_from_this(), pScheduler);
}

TimerHandle ScopedActor::PostDelay(const ActorSchedulerRef& pScheduler, const int64 delayMs, CallbackType&& callback)
{
	const auto pJob = cpp_net_engine::MakeShared<Job>(std::move(callback));
	TimerHandle handle = JobDispatcher::PostDelay(pJob, shared_from_this(), pScheduler, delayMs);
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
