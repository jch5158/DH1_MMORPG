#pragma once
#include "TimingWheel.h"

class JobDispatcher
{
public:

	JobDispatcher() = delete;
	~JobDispatcher() = delete;

	static void Post(const JobRef& pJob, const IActorRef& pActor);
	static TimerHandle PostDelay(const JobRef& pJob, const IActorRef& pActor, const ActorSchedulerRef& pScheduler, const int64 delayMs);
};

