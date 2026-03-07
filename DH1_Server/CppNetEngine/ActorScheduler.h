#pragma once
#include <functional>

#include "Job.h"
#include "TimingWheel.h"
#include "LockFreeQueue.h"

class ActorScheduler final : public std::enable_shared_from_this<ActorScheduler>
{
public:

	static constexpr int32 DEFAULT_EXECUTE_JOB_COUNT = 50;
	static constexpr int64 DEFAULT_TIME_SLICE_MS = 16;
	static constexpr int64 DEFAULT_TICK_INTERVAL_MS = 16;

	ActorScheduler(const ActorScheduler&) = delete;
	ActorScheduler& operator=(const ActorScheduler&) = delete;
	ActorScheduler(ActorScheduler&&) = delete;
	ActorScheduler& operator=(ActorScheduler&&) = delete;

	explicit ActorScheduler(std::function<void(const uint32)> pOnHandleError,
		const uint32 timeSliceMs = DEFAULT_TIME_SLICE_MS,
		const int32 executeJobCount = DEFAULT_EXECUTE_JOB_COUNT,
		const int64 tickIntervalMs = DEFAULT_TICK_INTERVAL_MS);
	~ActorScheduler();

	void Schedule(const IActorRef& pActor, const bool bBypassAcquire = false) const;
	TimerHandle ScheduleDelay(JobRef pJob, IActorRef pOwner, const uint64 delayMs);
	void Dispatch();

private:

	HANDLE mActorIocpHandle;
	const uint32 mTimeSliceMs;
	const int32 mExecuteJobCount;
	TimingWheel mTimingWheel;
	std::function<void(const uint32)> mpOnHandleError;
};