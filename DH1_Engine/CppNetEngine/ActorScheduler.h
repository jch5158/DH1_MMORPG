#pragma once
#include "IocpCore.h"
#include "Message.h"
#include "TimingWheel.h"

class ActorEvent;

class ActorScheduler final : public IocpCore
{
public:

	static constexpr int32 DEFAULT_EXECUTE_Message_COUNT = 50;
	static constexpr int64 DEFAULT_TIME_SLICE_MS = 16;
	static constexpr int64 DEFAULT_TICK_INTERVAL_MS = 16;

	ActorScheduler(const ActorScheduler&) = delete;
	ActorScheduler& operator=(const ActorScheduler&) = delete;
	ActorScheduler(ActorScheduler&&) = delete;
	ActorScheduler& operator=(ActorScheduler&&) = delete;

	explicit ActorScheduler(std::function<void(const uint32)> onHandleError,
		const uint32 timeSliceMs = DEFAULT_TIME_SLICE_MS,
		const int32 maxExecuteMessageCount = DEFAULT_EXECUTE_Message_COUNT,
		const int64 tickIntervalMs = DEFAULT_TICK_INTERVAL_MS);
	virtual ~ActorScheduler() override;

	[[nodiscard]] int32 GetMaxExecuteMessageCount() const;

	virtual bool Register(const IocpObjectRef& pIocpObject) override;
	virtual TimerHandle RegisterDelay(std::function<void()> delayFunction, const uint64 delayMs) override;
	virtual void Dispatch() override;

private:

	HANDLE mActorIocpHandle;
	const uint32 mTimeSliceMs;
	const int32 mMaxExecuteMessageCount;
	TimingWheel mTimingWheel;
	std::function<void(const uint32)> mOnHandleError;
};
