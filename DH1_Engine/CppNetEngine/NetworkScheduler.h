#pragma once

struct NetworkSchedulerConfig
{
	uint32 runningThreadCount = 0;
	uint32 waitTimeoutMs = 16;
	uint32 tickIntervalMs = 16;
	std::function<void(const uint32)> onHandleError = nullptr;
};

class NetworkScheduler final : public IocpCore
{
public:

	static constexpr int64 DEFAULT_TIME_SLICE_MS = 16;

	using IocpCore::Register;

	explicit NetworkScheduler(const NetworkSchedulerConfig& config);
	virtual ~NetworkScheduler() override = default;

	virtual void Dispatch() override;
	[[nodiscard]] virtual bool Register(const IocpObjectRef& pIocpObject) override;
	virtual TimerHandle RegisterDelay(std::function<void()> delayFunction, const uint64 delayMs) override;

private:
	const uint32 mWaitTimeoutMs;
	TimingWheel mTimingWheel;
	const std::function<void(const uint32)> mOnHandleError;
};

