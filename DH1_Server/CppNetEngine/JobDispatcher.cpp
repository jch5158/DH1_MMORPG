#include "pch.h"
#include "JobDispatcher.h"

void JobDispatcher::Post(const JobRef& pJob, const IActorRef& pActor)
{
	if (pJob == nullptr || pActor == nullptr)
	{
		return;
	}

	if (pActor->PushJob(pJob) == false)
	{
		NET_ENGINE_LOG_FATAL("JobDispatcher::Post - pActor->PushJob(pJob) is failed, jobCount : {}", pActor->GetJobCount());
		return;
	}

	pActor->Register();
}

TimerHandle JobDispatcher::PostDelay(const JobRef& pJob, const IActorRef& pActor, const ActorSchedulerRef& pScheduler, const int64 delayMs)
{
	if (pJob == nullptr || pActor == nullptr || pScheduler == nullptr)
	{
		TimerHandle handle;
		handle.Cancel();
		return handle;
	}

	TimerHandle handle = pScheduler->ScheduleDelay(pJob, pActor, delayMs);
	return handle;
}
