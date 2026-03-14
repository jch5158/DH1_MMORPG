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

bool IocpCore::Register(IocpEvent& iocpEvent)
{
	if (PostQueuedCompletionStatus(mIocpHandle, 0, 0, static_cast<LPOVERLAPPED>(&iocpEvent)) == false)
	{
		return false;
	}

	return true;
}

bool IocpCore::IsIgnorableError(const uint32 errorCode)
{
	switch (errorCode)
	{
	case WSA_IO_PENDING:
	case ERROR_NETNAME_DELETED:
	case ERROR_OPERATION_ABORTED:
	case WSAECONNRESET:
	case WSAECONNABORTED:
	case WAIT_TIMEOUT:
		return true;
	default:
		return false;
	}
}
