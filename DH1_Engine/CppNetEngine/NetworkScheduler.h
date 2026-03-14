#pragma once
#include "IocpCore.h"
#include "TimingWheel.h"

class NetworkScheduler final : public IocpCore
{
public:

	using IocpCore::Register;

	explicit NetworkScheduler(const uint32 waitTimeoutMs, std::function<void(const uint32)> onHandleError);
	virtual ~NetworkScheduler() override = default;

	virtual void Dispatch() override;
	[[nodiscard]] virtual bool Register(const IocpObjectRef& pIocpObject) override;
	virtual TimerHandle RegisterDelay(std::function<void()> delayFunction, const uint64 delayMs) override;

private:
	const uint32 mWaitTimeoutMs;
	const std::function<void(const uint32)> mOnHandleError;
	TimingWheel mTimingWheel;
};

