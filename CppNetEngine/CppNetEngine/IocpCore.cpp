#include "pch.h"
#include "IocpEvent.h"
#include "IocpCore.h"
#include "CrashReporter.h"

IocpCore::IocpCore()
	: mIocpHandle(nullptr)
	, mpOnHandleError([](const uint32)->void {return; })
{
	mIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (mIocpHandle == nullptr)
	{
		CrashReporter::Crash();
	}
}

IocpCore::IocpCore(std::function<void(const uint32)> pOnHandleError)
	: mIocpHandle(nullptr)
	, mpOnHandleError(std::move(pOnHandleError))
{
	mIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (mIocpHandle == nullptr)
	{
		CrashReporter::Crash();
	}
}

IocpCore::~IocpCore()
{
	if (mIocpHandle != nullptr)
	{
		CloseHandle(mIocpHandle);
	}
}

HANDLE IocpCore::GetHandle() const
{
	return mIocpHandle;
}

bool IocpCore::Register(const IocpObjectRef& iocpObject) const
{
	if (nullptr == CreateIoCompletionPort(iocpObject->GetHandle(), mIocpHandle, 0, 0))
	{
		return false;
	}

	return true;
}

void IocpCore::Dispatch(const uint32 timeout) const
{
	uint32 numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* pIocpEvent = nullptr;
	const int32 gqcsRet = GetQueuedCompletionStatus(mIocpHandle, reinterpret_cast<LPDWORD>(&numOfBytes), &key, reinterpret_cast<LPOVERLAPPED*>(&pIocpEvent), timeout);
	if (gqcsRet == 0)
	{
		const uint32 errorCode = GetLastError();
		if (errorCode != WAIT_TIMEOUT)
		{
			mpOnHandleError(errorCode);
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
}
