#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager(const int32 maxSessionCount)
	: mMaxSessionCount(maxSessionCount)
	, mSessions()
{
}

bool SessionManager::AddSession(SessionRef pSession)
{
	UniqueLock lock(mLock);

	if (std::cmp_less_equal(mMaxSessionCount, mSessions.size()))
	{
		return false;
	}

	const int64 retVal = mSessions.emplace(std::move(pSession)).second;
	return retVal;
}

void SessionManager::RemoveSession(const SessionRef& pSession)
{
	UniqueLock lock(mLock);
	mSessions.erase(pSession);
}

int32 SessionManager::GetMaxSessionCount() const
{
	return mMaxSessionCount;
}

int32 SessionManager::GetCurrentSessionCount()
{
	UniqueLock lock(mLock);
	return static_cast<int32>(mSessions.size());
}
