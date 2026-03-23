#pragma once
#include "IocpCore.h"

struct ActorSchedulerConfig
{
	uint32 runningThreadCount = 0;
	uint32 waitTimeoutMs = 16;
	uint32 tickIntervalMs = 16;
	std::function<void(const uint32)> onHandleError = nullptr;
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

private:

	const uint32 mWaitTimeoutMs;
	TimingWheel mTimingWheel;
	std::function<void(const uint32)> mOnHandleError;
};
