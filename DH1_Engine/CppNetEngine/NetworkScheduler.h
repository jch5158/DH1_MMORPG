#pragma once

struct NetworkSchedulerConfig
{
	uint32 runningThreadCount;
	uint32 waitTimeoutMs;
	uint32 tickIntervalMs;
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

	void ShutdownScheduler(const uint32 dispatchThreadCount);
	[[nodiscard]] bool IsStopped() const;

private:
	const uint32 mWaitTimeoutMs;
	std::atomic<bool> mbStopped;
	TimingWheel mTimingWheel;
};

