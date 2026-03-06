#pragma once
#include "LockFreeStack.h"
#include "SessionReaper.h"

class SessionManager
{
public:
	SessionManager(const SessionManager&) = delete;
	SessionManager operator=(const SessionManager&) = delete;
	SessionManager(SessionManager&&) = delete;
	SessionManager operator=(SessionManager&&) = delete;

	explicit SessionManager(const int32 maxSessionCount);
	~SessionManager() = default;

	[[nodiscard]] bool AddSession(const SessionRef& pSession);
	[[nodiscard]] bool AddWaitingSession(const SessionRef& pSession);
	void RemoveSession(const SessionRef& pSession, const bool bKeepWaitingSession = false);
	void ReleaseKeepTicket();

	[[nodiscard]] int32 GetMaxSessionCount() const;
	[[nodiscard]] int32 GetCurrentSessionCount();

private:

	const int32 mMaxSessionCount;

	Mutex mLock;
	int32 mCurrentSessionCount;
	Set<SessionRef> mSessions;
};
