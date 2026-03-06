#include "pch.h"
#include "WaitQueueManager.h"

#include <utility>

WaitQueueManager::WaitQueueManager(const int32 maxWaitSize)
	: mLock()
	, mMaxWaitSize(maxWaitSize)
	, mWaitTicket(0)
	, mEnterTicket(0)
	, mEnterWaitQueue()
{
}

bool WaitQueueManager::EnterWaitQueue(const SessionRef& pSession, uint64& outTicket)
{
	UniqueLock lock(mLock);
	if (std::cmp_less_equal(mMaxWaitSize, mEnterWaitQueue.size()))
	{
		return false;
	}
	
	mEnterWaitQueue.push(pSession);
	outTicket = mWaitTicket++;
	return true;
}

SessionRef WaitQueueManager::DequeueWaitQueue()
{
	UniqueLock lock(mLock);

	while (!mEnterWaitQueue.empty())
	{
		++mEnterTicket;

		const SessionWeak pSessionWeak = mEnterWaitQueue.front();
		mEnterWaitQueue.pop();

		SessionRef pSession = pSessionWeak.lock();
		if (pSession == nullptr)
		{
			continue;
		}

		return pSession;
	}

	return nullptr;
}

uint64 WaitQueueManager::GetWaitCount(const uint64 myTicket)
{
	UniqueLock lock(mLock);

	if (myTicket > mEnterTicket)
	{
		return myTicket - mEnterTicket;
	}

	return 0;
}