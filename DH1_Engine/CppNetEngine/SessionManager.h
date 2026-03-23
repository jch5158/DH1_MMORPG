#pragma once

class SessionManager
{
public:
	SessionManager(const SessionManager&) = delete;
	SessionManager operator=(const SessionManager&) = delete;
	SessionManager(SessionManager&&) = delete;
	SessionManager operator=(SessionManager&&) = delete;

	explicit SessionManager();
	~SessionManager() = default;

	[[nodiscard]] bool AddSession(SessionRef pSession);
	void RemoveSession(const SessionRef& pSession);
	
	void SetMaxSessionCount(const int32 maxSessionCount);
	[[nodiscard]] int32 GetMaxSessionCount() const;
	[[nodiscard]] int32 GetCurrentSessionCount();
	[[nodiscard]] SessionRef GetFirstSessionRef();

private:
	int32 mMaxSessionCount;

	SharedMutex mLock;
	Set<SessionRef> mSessions;
};
