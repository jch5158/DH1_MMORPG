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

void ThreadManager::Launch(std::function<void()> callback)
{
	LockGuard guard(mLock);

	mThreads.emplace_back([argCallback = std::move(callback)]()->void
	{
		ThreadManager::InitTls();
		argCallback();
		ThreadManager::DestroyTls();
	});
}

void ThreadManager::JoinWithClear()
{
	for (auto& thread : mThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	mThreads.clear();
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
