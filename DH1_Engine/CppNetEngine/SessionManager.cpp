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

	const uint64 sessionId = pSession->GetSessionId();
	const bool retVal = mSessions.try_emplace(sessionId, std::move(pSession)).second;
	return retVal;
}

bool SessionManager::Contains(const SessionRef& pSession)
{
	SharedLock lock(mLock);
	return mSessions.contains(pSession->GetSessionId());
}

void SessionManager::RemoveSession(const SessionRef& pSession)
{
	if (pSession == nullptr)
	{
		return;
	}

	const uint64 sessionId = pSession->GetSessionId();

	UniqueLock lock(mLock);
	mSessions.erase(sessionId);
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
	SharedLock lock(mLock);
	return static_cast<int32>(mSessions.size());
}

SessionRef SessionManager::GetFirstSessionRef()
{
	SharedLock lock(mLock);
	if (mSessions.begin() == mSessions.end())
	{
		return nullptr;
	}

	return mSessions.begin()->second;
}

SessionRef SessionManager::GetSession(const uint64 sessionId)
{
	SharedLock lock(mLock);
	const auto iter = mSessions.find(sessionId);
	if (iter == mSessions.end())
	{
		return nullptr;
	}

	return iter->second;
}

Vector<SessionRef> SessionManager::GetActiveSessions()
{
	UniqueLock lock(mLock);

	Vector<SessionRef> sessions;
	sessions.reserve(mSessions.size());
	for (const auto& pSession : mSessions | std::views::values)
	{
		sessions.emplace_back(pSession);
	}

	return sessions;
}
