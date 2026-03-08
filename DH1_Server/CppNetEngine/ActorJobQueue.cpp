#include "pch.h"
#include "ActorJobQueue.h"

ActorJobQueue::ActorJobQueue()
	: mJobActorEvent()
	, mpScheduler(nullptr)
	, mJobQueue()
{
}

void ActorJobQueue::Initialize(const IActorRef& pOwner, ActorSchedulerRef pScheduler)
{
	if (pOwner == nullptr || pScheduler == nullptr)
	{
		return;
	}

	mJobActorEvent.SetOwner(pOwner);
	mpScheduler = std::move(pScheduler);
}

bool ActorJobQueue::PushJob(JobRef pJob)
{
	return mJobQueue.TryEnqueue(std::move(pJob));
}

int32 ActorJobQueue::GetJobCount() const
{
	return mJobQueue.Count();
}

void ActorJobQueue::Process()
{
	const IActorRef pActor = mJobActorEvent.GetOwner();
	if (pActor != nullptr)
	{
		if (pActor->TryAcquire())
		{
			const int32 currentJobCount = mJobQueue.Count();
			const int32 maxExecuteJobCount = mpScheduler->GetMaxExecuteJobCount();
			const int32 executeJobCount = maxExecuteJobCount < currentJobCount ? maxExecuteJobCount : currentJobCount;

			for (int32 i = 0; i < executeJobCount; ++i)
			{
				JobRef pJob;

				if (mJobQueue.TryDequeue(pJob))
				{
					pJob->Execute();
				}
				else
				{
					break;
				}
			}

			pActor->Release();
		}

		Register();
	}
}

void ActorJobQueue::Register()
{
	const IActorRef pActor = mJobActorEvent.GetOwner();
	if (pActor->TryAcquire())
	{
		if (mJobQueue.Count() > 0)
		{
			if (mpScheduler != nullptr)
			{
				mpScheduler->Schedule(mJobActorEvent);
			}
		}

		pActor->Release();
	}
}

void ActorJobQueue::Flush()
{
	JobRef pJob;
	while (mJobQueue.TryDequeue(pJob))
	{
		if (pJob != nullptr)
		{
			pJob->Execute();
		}
	}
}