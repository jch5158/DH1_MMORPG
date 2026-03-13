#pragma once
#include "Actor.h"

class SessionReaper final
{
public:

	static constexpr int64 DEFAULT_TIME_OUT = 60000;

	explicit SessionReaper(ActorSchedulerRef pScheduler, const int64 timeoutMs);
	~SessionReaper() = default;

	[[nodiscard]] int64 GetTimeoutMs() const;
	void ReapSession(const SessionWeak& pSessionWeak) const;

private:

	bool isExpired(const int64 lastActivityMs) const;
	static int64 getNowTimeMs();

	const int64 mTimeoutMs;
};

