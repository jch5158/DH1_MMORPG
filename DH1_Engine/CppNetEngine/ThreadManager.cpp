#include "pch.h"
#include "ThreadManager.h"

#include "Actor.h"
#include "ActorScheduler.h"

ThreadManager::ThreadManager()
	: ISingleton<ThreadManager>()
	, mLock()
	, mThreads()
{
	InitTls();
}

ThreadManager::~ThreadManager()
{
	JoinWithClear();
}

void ThreadManager::Launch(const std::string& threadName, std::function<void()> callback)
{
	LockGuard guard(mLock);

	mThreads.emplace_back([threadName, argCallback = std::move(callback)]()->void
		{
			ThreadManager::InitTls();

			const std::wstring threadNameWithTag = std::format(L"{}_{}", cpp_net_engine::ToWstring(threadName), sTlsThreadId);
			const HRESULT result = SetThreadDescription(GetCurrentThread(), threadNameWithTag.c_str());
			if (FAILED(result))
			{
				NET_ENGINE_LOG_FATAL("ThreadManager::Launch - SetThreadDescription Failed, HRESULT : {:#X}", static_cast<uint32>(result));
				CrashReporter::Crash();
			}

			argCallback();

			ThreadManager::DestroyTls();
		});
}

void ThreadManager::JoinWithClear()
{
	Vector<std::thread> localThreads;

	{
		LockGuard guard(mLock);
		localThreads = std::move(mThreads);
	}

	for (auto& thread : localThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void ThreadManager::InitTls()
{
	static std::atomic<uint32> sThreadId = 1;
	sTlsThreadId = sThreadId.fetch_add(1);
}

void ThreadManager::DestroyTls()
{
	sTlsThreadId = 0;
}

uint32 ThreadManager::GetThreadId()
{
	if (sTlsThreadId == 0)
	{
		InitTls();
	}

	return sTlsThreadId;
}

thread_local uint32 ThreadManager::sTlsThreadId = 0;
