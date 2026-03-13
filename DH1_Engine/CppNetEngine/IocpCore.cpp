#include "pch.h"
#include "IocpEvent.h"
#include "IocpCore.h"
#include "CrashReporter.h"

IocpCore::IocpCore()
	: mIocpHandle(nullptr)
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
