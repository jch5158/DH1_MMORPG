#pragma once
#include "IocpCore.h"

struct ActorSchedulerConfig
{
	uint32 runningThreadCount = 0;
	uint32 waitTimeoutMs = 16;
	uint32 tickIntervalMs = 16;
};

class ActorScheduler final : public IocpCore
{
public:

	using IocpCore::Register;

	static constexpr int64 DEFAULT_TIME_SLICE_MS = 16;
	static constexpr int64 DEFAULT_TICK_INTERVAL_MS = 16;

	ActorScheduler(const ActorScheduler&) = delete;
	ActorScheduler& operator=(const ActorScheduler&) = delete;
	ActorScheduler(ActorScheduler&&) = delete;
	ActorScheduler& operator=(ActorScheduler&&) = delete;

	explicit ActorScheduler(const ActorSchedulerConfig& config);
	virtual ~ActorScheduler() override = default;

	virtual bool Register(const IocpObjectRef& pIocpObject) override;
	virtual TimerHandle RegisterDelay(std::function<void()> delayFunction, const uint64 delayMs) override;
	virtual void Dispatch() override;

	void ShutdownScheduler(const uint32 dispatchThreadCount);
	[[nodiscard]] bool IsStopped() const;

private:

	const uint32 mWaitTimeoutMs;
	std::atomic<bool> mbStopped;
	TimingWheel mTimingWheel;
};
