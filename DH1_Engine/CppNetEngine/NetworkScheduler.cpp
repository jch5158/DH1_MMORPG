#include "pch.h"
#include "NetworkScheduler.h"

#include "IocpEvent.h"
#include "SocketIocpObject.h"
#include "SocketUtils.h"

NetworkScheduler::NetworkScheduler(const NetworkSchedulerConfig& config)
	: IocpCore(config.runningThreadCount)
	, mWaitTimeoutMs(config.waitTimeoutMs)
	, mTimingWheel(config.tickIntervalMs)
{
}

void NetworkScheduler::Dispatch()
{
	uint32 numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* pIocpEvent = nullptr;
	const int32 gqcsRet = GetQueuedCompletionStatus(mIocpHandle, reinterpret_cast<LPDWORD>(&numOfBytes), &key, reinterpret_cast<LPOVERLAPPED*>(&pIocpEvent), mWaitTimeoutMs);
	if (gqcsRet == 0)
	{
		const uint32 errorCode = GetLastError();
		if (!SocketUtils::IsExpectedIocpError(errorCode))
		{
			NET_ENGINE_LOG_ERROR("NetworkScheduler::Dispatch - GQCS failed, errorCode : {}", errorCode);
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

	//mTimingWheel.Tick();
}

bool NetworkScheduler::Register(const IocpObjectRef& pIocpObject)
{
	const auto pSocketIocpObj = std::static_pointer_cast<SocketIocpObject>(pIocpObject);
	if (nullptr == CreateIoCompletionPort(pSocketIocpObj->GetSocketHandle(), mIocpHandle, 0, 0))
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
