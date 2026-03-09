#pragma once
#include "ActorEvent.h"
#include "LockFreeQueue.h"

class ActorJobQueue
{
public:

	ActorJobQueue(const ActorJobQueue&) = delete;
	ActorJobQueue& operator=(const ActorJobQueue&) = delete;
	ActorJobQueue(ActorJobQueue&&) = delete;
	ActorJobQueue& operator=(ActorJobQueue&&) = delete;

	explicit ActorJobQueue();
	~ActorJobQueue() = default;

	void Initialize(const IActorRef& pOwner, ActorSchedulerRef pScheduler);

	bool PushJob(JobRef pJob);
	[[nodiscard]] int32 GetJobCount() const;
	void Process();
	void Register();
	void Flush();

private:

	JobActorEvent mJobActorEvent;
	ActorSchedulerRef mpScheduler;
	LockFreeQueue<JobRef> mJobQueue;
};

