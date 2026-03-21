#pragma once

#include "ISingleton.h"
#include "StlTypes.h"
#include "Types.h"

class ThreadManager final : public ISingleton<ThreadManager>
{
public:
	friend class ISingleton<ThreadManager>;

	ThreadManager(const ThreadManager&) = delete;
	ThreadManager& operator=(const ThreadManager&) = delete;
	ThreadManager(ThreadManager&&) = delete;
	ThreadManager& operator=(ThreadManager&&) = delete;

private:

	explicit ThreadManager();

public:

	~ThreadManager();

	void Launch(const std::string& threadName, std::function<void()> callback);
	void JoinWithClear();

	static void InitTls();
	static void DestroyTls();

	[[nodiscard]]
	static uint32 GetThreadId();

private:

	Mutex mLock;
	Vector<std::thread> mThreads;

	static thread_local uint32 sTlsThreadId;
	static thread_local std::chrono::time_point<std::chrono::steady_clock> sTlsMessageWorkEndTime;
};

