#include "pch.h"
#include "SessionManager.h"
#include "Session.h"

SessionManager::SessionManager(const int32 maxSessionCount)
	: mMaxSessionCount(maxSessionCount)
	, mCurrentSessionCount(0)
	, mSessions()
{
}

bool SessionManager::AddSession(SessionRef pSession)
{
	UniqueLock lock(mLock);

	if (mCurrentSessionCount >= mMaxSessionCount)
	{
		return false;
	}

	if (mSessions.emplace(std::move(pSession)).second)
	{
		++mCurrentSessionCount;
		return true;
	}

	return false;
}

bool SessionManager::AddWaitingSession(SessionRef pSession)
{
	UniqueLock lock(mLock);
	return mSessions.emplace(std::move(pSession)).second;
}

void SessionManager::RemoveSession(const SessionRef& pSession, const bool bKeepWaitingSession)
{
	UniqueLock lock(mLock);
	if (mSessions.erase(pSession) != 0)
	{
		if (bKeepWaitingSession == false)
		{
			--mCurrentSessionCount;
		}
	}
}

void SessionManager::ReleaseKeepTicket()
{
	UniqueLock lock(mLock);
	--mCurrentSessionCount;
}

int32 SessionManager::GetMaxSessionCount() const
{
	return mMaxSessionCount;
}

int32 SessionManager::GetCurrentSessionCount()
{
	UniqueLock lock(mLock);
	return mCurrentSessionCount;
}
