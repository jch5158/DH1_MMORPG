#include "pch.h"
#include "SessionReaper.h"
#include "NetService.h"
#include "Session.h"

SessionReaper::SessionReaper(const int64 timeoutMs)
	: mTimeoutMs(timeoutMs)
{
}

int64 SessionReaper::GetTimeoutMs() const
{
	return mTimeoutMs;
}

void SessionReaper::Sweep(const ServerServiceWeak& pServerServiceWeak)
{
	const ServerServiceRef pServerService = pServerServiceWeak.lock();
	if (pServerService == nullptr)
	{
		return;
	}

	Vector<SessionRef> sessions = pServerService->GetActiveSessions();
	const int64 now = getNowTimeMs();

	for (const auto& pSession : sessions)
	{
		if (pSession == nullptr || pSession->IsDisconnectStarted())
		{
			continue;
		}

		if (isExpired(pSession->getLastActivityMs()))
		{
			pSession->Disconnect(eDisconnectReason::Timeout);

			UniqueLock lock(mAbortLock);
			mAbortPendingSessions.push_back({ SessionWeak(pSession), now });
		}
	}
}

void SessionReaper::SweepAbort()
{
	UniqueLock lock(mAbortLock);

	auto iter = mAbortPendingSessions.begin();
	while (iter != mAbortPendingSessions.end())
	{
		const SessionRef pSession = iter->pSessionWeak.lock();
		if (pSession == nullptr || pSession->IsDisconnected())
		{
			iter = mAbortPendingSessions.erase(iter);
			continue;
		}

		if (isAbortExpired(iter->disconnectStartMs))
		{
			pSession->ForceCloseSocket();
			iter = mAbortPendingSessions.erase(iter);
			continue;
		}

		++iter;
	}
}

bool SessionReaper::isExpired(const int64 lastActivityMs) const
{
	return (getNowTimeMs() - lastActivityMs) > mTimeoutMs;
}

bool SessionReaper::isAbortExpired(const int64 disconnectStartMs) const
{
	return (getNowTimeMs() - disconnectStartMs) > ABORT_TIMEOUT_MS;
}

int64 SessionReaper::getNowTimeMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}
