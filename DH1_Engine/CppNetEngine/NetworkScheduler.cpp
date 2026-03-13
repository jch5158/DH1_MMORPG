#include "pch.h"
#include "NetworkScheduler.h"
#include "IocpEvent.h"
#include "SessionReaper.h"
#include "SocketIocpObject.h"

NetworkScheduler::NetworkScheduler(const uint32 waitTimeoutMs, std::function<void(const uint32)> onHandleError)
	: IocpCore()
	, mWaitTimeoutMs(waitTimeoutMs)
	, mOnHandleError(std::move(onHandleError))
	, mTimingWheel()
{
}

void NetworkScheduler::Dispatch()
{
	uint32 numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* pIocpEvent = nullptr;
	const int32 gqcsRet = GetQueuedCompletionStatus(GetHandle(), reinterpret_cast<LPDWORD>(&numOfBytes), &key, reinterpret_cast<LPOVERLAPPED*>(&pIocpEvent), mWaitTimeoutMs);
	if (gqcsRet == 0)
	{
		const uint32 errorCode = GetLastError();
		if (errorCode != WAIT_TIMEOUT)
		{
			mOnHandleError(errorCode);
		}
	}

	if (pIocpEvent != nullptr)
	{
		const IocpObjectRef pIocpObj = pIocpEvent->GetOwner();
		if (pIocpObj != nullptr)
		{
			pIocpObj->Dispatch(*pIocpEvent, numOfBytes);
		}
	}

	mTimingWheel.Tick();
}

bool NetworkScheduler::Register(const IocpObjectRef& pIocpObject)
{
	const auto pSocketIocpObj = std::static_pointer_cast<SocketIocpObject>(pIocpObject);
	if (nullptr == CreateIoCompletionPort(pSocketIocpObj->GetSocketHandle(), GetHandle(), 0, 0))
	{
		return false;
	}

	return true;
}

TimerHandle NetworkScheduler::RegisterDelay(std::function<void()> delayFunction, const uint64 delayMs)
{
	TimerHandle handle = mTimingWheel.AddTiming([capFunction = std::move(delayFunction)]()->void
		{
			capFunction();
		}, delayMs);

	return handle;
}
