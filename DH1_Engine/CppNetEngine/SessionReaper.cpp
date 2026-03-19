#include "pch.h"
#include "SessionReaper.h"
#include "Session.h"
#include "SocketUtils.h"

SessionReaper::SessionReaper(const int64 timeoutMs)
	: mTimeoutMs(timeoutMs)
{
}

int64 SessionReaper::GetTimeoutMs() const
{
	return mTimeoutMs;
}

void SessionReaper::ReapSession(const ServerServiceWeak& pServerServiceWeak, const SessionWeak& pSessionWeak) const
{
	const ServerServiceRef pServerService = pServerServiceWeak.lock();
	if (pServerService == nullptr)
	{
		return;
	}

	const SessionRef pSession = pSessionWeak.lock();
	if (pSession == nullptr || pSession->IsDisconnectStarted())
	{
		return;
	}

	if (isExpired(pSession->getLastActivityMs()))
	{
		pSession->Disconnect(eDisconnectReason::Timeout);
	}
	else
	{
		pServerService->RegisterSessionReap(pSession);
	}
}

void SessionReaper::AbortSession(const SessionWeak& pSessionWeak)
{
	const SessionRef pSession = pSessionWeak.lock();
	if (pSession == nullptr || !pSession->IsDisconnecting())
	{
		return;
	}

	pSession->ForceCloseSocket();
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