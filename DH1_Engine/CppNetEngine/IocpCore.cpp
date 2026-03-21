#include "pch.h"
#include "IocpCore.h"
#include "CrashReporter.h"
#include "IocpEvent.h"

IocpCore::IocpCore(const uint32 runningThreadCount)
	: mIocpHandle(nullptr)
	, mRunningThreadCount(runningThreadCount)
{
	mIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, runningThreadCount);
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

bool IocpCore::Register(IocpEvent& iocpEvent)
{
	if (PostQueuedCompletionStatus(mIocpHandle, 0, 0, static_cast<LPOVERLAPPED>(&iocpEvent)) == false)
	{
		return false;
	}

	return true;
}

uint32 IocpCore::GetRunningThreadCount() const
{
	return mRunningThreadCount;
}
