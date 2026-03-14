#pragma once
#include "TimingWheel.h"

class IocpObject : public std::enable_shared_from_this<IocpObject>
{
public:
	IocpObject(const IocpObject&) = delete;
	IocpObject& operator=(const IocpObject&) = delete;
	IocpObject(IocpObject&&) = delete;
	IocpObject& operator=(IocpObject&&) = delete;

	IocpObject() = default;
	virtual ~IocpObject() = default;

	virtual void Dispatch(class IocpEvent& iocpEvent, const uint32 numOfBytes) = 0;
};

class IocpCore : public std::enable_shared_from_this<IocpCore>
{
public:
	IocpCore(const IocpCore&) = delete;
	IocpCore& operator=(const IocpCore&) = delete;
	IocpCore(IocpCore&&) = delete;
	IocpCore& operator=(IocpCore&&) = delete;

	explicit IocpCore();
	virtual ~IocpCore();

	[[nodiscard]] HANDLE GetHandle() const;

	virtual void Dispatch() = 0;
	[[nodiscard]] virtual bool Register(const IocpObjectRef& pIocpObject) = 0;
	[[nodiscard]] virtual bool Register(IocpEvent& iocpEvent);
	virtual TimerHandle RegisterDelay(std::function<void()> delayFunction, const uint64 delayMs) = 0;

protected:

	HANDLE mIocpHandle;
};

