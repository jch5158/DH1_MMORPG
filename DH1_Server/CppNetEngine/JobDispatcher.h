#pragma once
#include "TimingWheel.h"

class JobDispatcher
{
public:

	JobDispatcher() = delete;
	~JobDispatcher() = delete;

	static void Post(JobRef pJob, const IActorRef& pActor);
	static TimerHandle PostDelay(JobRef pJob, IActorRef pActor, const ActorSchedulerRef& pScheduler, const int64 delayMs);
};

