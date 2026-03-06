#pragma once

class SessionTimeoutTracker
{
public:

	static constexpr int64 ONE_SECOND_MS = 1000;

	SessionTimeoutTracker(const SessionTimeoutTracker&) = delete;
	SessionTimeoutTracker operator=(const SessionTimeoutTracker&) = delete;
	SessionTimeoutTracker(SessionTimeoutTracker&&) = delete;
	SessionTimeoutTracker operator=(SessionTimeoutTracker&&) = delete;

	explicit SessionTimeoutTracker();
	~SessionTimeoutTracker() = default;

	void UpdateActivity();
	[[nodiscard]] int64 GetLastActivityMs() const;
	
private:
	
	static int64 getNowTimeMs();
	
	std::atomic<int64> mLastActivityMs;
};

