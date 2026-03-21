#pragma once
#include "SharedPtrUtils.h"
#include "StlTypes.h"
#include "Types.h"

class SessionManager
{
public:
	SessionManager(const SessionManager&) = delete;
	SessionManager operator=(const SessionManager&) = delete;
	SessionManager(SessionManager&&) = delete;
	SessionManager operator=(SessionManager&&) = delete;

	explicit SessionManager(const int32 maxSessionCount);
	~SessionManager() = default;

	[[nodiscard]] bool AddSession(SessionRef pSession);
	void RemoveSession(const SessionRef& pSession);
	
	[[nodiscard]] int32 GetMaxSessionCount() const;
	[[nodiscard]] int32 GetCurrentSessionCount();

private:

	const int32 mMaxSessionCount;

	SharedMutex mLock;
	Set<SessionRef> mSessions;
};
