#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager()
	: mMaxSessionCount(0)
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

void SessionManager::SetMaxSessionCount(const int32 maxSessionCount)
{
	mMaxSessionCount = maxSessionCount;
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

SessionRef SessionManager::GetFirstSessionRef()
{
	UniqueLock lock(mLock);
	if (mSessions.begin() == mSessions.end())
	{
		return nullptr;
	}

	return *mSessions.begin();
}
