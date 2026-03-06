#include "pch.h"
#include "SessionTimeoutTracker.h"

SessionTimeoutTracker::SessionTimeoutTracker()
	: mLastActivityMs(getNowTimeMs())
{
}

void SessionTimeoutTracker::UpdateActivity()
{
	const auto now = getNowTimeMs();
	if (now - mLastActivityMs.load() > ONE_SECOND_MS)
	{
		mLastActivityMs.store(now);
	}
}

int64 SessionTimeoutTracker::GetLastActivityMs() const
{
	return mLastActivityMs.load();
}

int64 SessionTimeoutTracker::getNowTimeMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
