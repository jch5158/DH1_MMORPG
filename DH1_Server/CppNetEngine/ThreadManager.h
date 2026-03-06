#pragma once

#include "ISingleton.h"
#include <functional>

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

	virtual ~ThreadManager() override;

	void Launch(std::function<void()> callback);
	void JoinWithClear();

	static void InitTls();
	static void DestroyTls();

	[[nodiscard]]
	static uint32 GetThreadId();

private:

	Mutex mLock;
	Vector<std::thread> mThreads;

	static thread_local uint32 sTlsThreadId;
	static thread_local std::chrono::time_point<std::chrono::steady_clock> sTlsJobWorkEndTime;
};

