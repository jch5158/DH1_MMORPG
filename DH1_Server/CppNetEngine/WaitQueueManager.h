#pragma once
class WaitQueueManager
{
public:

	WaitQueueManager(const WaitQueueManager&) = delete;
	WaitQueueManager operator=(const WaitQueueManager&) = delete;
	WaitQueueManager(WaitQueueManager&&) = delete;
	WaitQueueManager operator=(WaitQueueManager&&) = delete;

	explicit WaitQueueManager(const int32 maxWaitSize);
	~WaitQueueManager() = default;

	[[nodiscard]] bool EnterWaitQueue(const SessionRef& pSession, uint64& outTicket);
	SessionRef DequeueWaitQueue();

	[[nodiscard]] uint64 GetWaitCount(const uint64 myTicket);

private:

	Mutex mLock;
	const int32 mMaxWaitSize;
	uint64 mWaitTicket;
	uint64 mEnterTicket;
	Queue<SessionWeak> mEnterWaitQueue;
};

