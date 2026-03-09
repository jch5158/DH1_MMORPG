#include "pch.h"
#include "JobDispatcher.h"

void JobDispatcher::Post(JobRef pJob, const IActorRef& pActor)
{
	if (pJob == nullptr || pActor == nullptr)
	{
		return;
	}

	if (pActor->PushJob(std::move(pJob)) == false)
	{
		NET_ENGINE_LOG_FATAL("JobDispatcher::Post - pActor->PushJob(pJob) is failed, jobCount : {}", pActor->GetJobCount());
		return;
	}

	pActor->Register();
}

TimerHandle JobDispatcher::PostDelay(JobRef pJob, IActorRef pActor, const ActorSchedulerRef& pScheduler, const int64 delayMs)
{
	if (pJob == nullptr || pActor == nullptr || pScheduler == nullptr)
	{
		TimerHandle handle;
		handle.Cancel();
		return handle;
	}

	TimerHandle handle = pScheduler->ScheduleDelay(std::move(pJob), std::move(pActor), delayMs);
	return handle;
}
