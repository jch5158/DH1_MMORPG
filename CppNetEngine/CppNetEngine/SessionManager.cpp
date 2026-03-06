#include "pch.h"
#include "SessionManager.h"
#include "Session.h"

SessionManager::SessionManager(const int32 maxSessionCount)
	: mMaxSessionCount(maxSessionCount)
	, mCurrentSessionCount(0)
	, mSessions()
{
}

bool SessionManager::AddSession(const SessionRef& pSession)
{
	UniqueLock lock(mLock);

	// 1. 최대 인원(예약석 포함) 검사 복구
	if (mCurrentSessionCount >= mMaxSessionCount)
	{
		return false;
	}

	// 2. 맵에 실제로 삽입이 성공했을 때만 카운트를 증가시킴 (누수 방지)
	if (mSessions.emplace(pSession).second)
	{
		++mCurrentSessionCount;
		return true;
	}

	return false;
}

bool SessionManager::AddWaitingSession(const SessionRef& pSession)
{
	UniqueLock lock(mLock);
	return mSessions.emplace(pSession).second;
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
