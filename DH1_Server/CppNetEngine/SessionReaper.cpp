#include "pch.h"
#include "SessionReaper.h"

#include "Session.h"

SessionReaper::SessionReaper(const ActorContext& actorContext, const int64 timeoutMs)
	: Actor(actorContext)
	, mTimeoutMs(timeoutMs)
{
}

int64 SessionReaper::GetTimeoutMs() const
{
	return mTimeoutMs;
}

void SessionReaper::ReapSession(const SessionWeak& pSessionWeak) const
{
	const SessionRef pSession = pSessionWeak.lock();
	if (pSession == nullptr)
	{
		return;
	}

	if (isExpired(pSession->OnGetLastActivityMs()))
	{
		pSession->Disconnect(eDisconnectReason::Timeout);
	}
	else
	{
		pSession->registerReap();
	}
}

bool SessionReaper::isExpired(const int64 lastActivityMs) const
{
	const auto now = getNowTimeMs();
	if (now - lastActivityMs > mTimeoutMs)
	{
		return true;
	}

	return false;
}

int64 SessionReaper::getNowTimeMs()
{
	const int64 nowTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	return nowTimeMs;
}