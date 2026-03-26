#include "pch.h"
#include "GameSessionManager.h"

GameSessionManager::GameSessionManager(const int32 maxSessionCount)
	: mMaxSessionCount(maxSessionCount)
{
	mGameSessions.reserve(maxSessionCount);
}

int32 GameSessionManager::GetMaxSessionCount() const
{
	return mMaxSessionCount;
}

int32 GameSessionManager::GetCurrentSessionCount() const
{
	SharedLock lock(mLock);
	return static_cast<int32>(mGameSessions.size());
}

bool GameSessionManager::AddSession(const GameSessionInfo& sessionInfo)
{
	UniqueLock lock(mLock);

	if (static_cast<int32>(mGameSessions.size()) >= mMaxSessionCount)
	{
		return false;
	}

	return mGameSessions.try_emplace(sessionInfo.mAccountId, sessionInfo).second;
}

bool GameSessionManager::RemoveSession(const uint64 accountId)
{
	UniqueLock lock(mLock);
	return mGameSessions.erase(accountId) > 0;
}

bool GameSessionManager::UpdateSession(const GameSessionInfo& sessionInfo)
{
	UniqueLock lock(mLock);

	const auto iter = mGameSessions.find(sessionInfo.mAccountId);
	if (iter == mGameSessions.end())
	{
		return false;
	}

	iter->second = sessionInfo;
	return true;
}

bool GameSessionManager::HasSession(const uint64 accountId) const
{
	SharedLock lock(mLock);
	return mGameSessions.contains(accountId);
}

std::optional<GameSessionInfo> GameSessionManager::GetSession(const uint64 accountId) const
{
	SharedLock lock(mLock);

	const auto iter = mGameSessions.find(accountId);
	if (iter == mGameSessions.end())
	{
		return std::nullopt;
	}

	return iter->second;
}

Vector<uint64> GameSessionManager::RemoveSessionsByGateway(const int32 gatewayServerId)
{
	UniqueLock lock(mLock);

	Vector<uint64> removedAccountIds;

	for (auto iter = mGameSessions.begin(); iter != mGameSessions.end();)
	{
		if (iter->second.mGatewayServerId == gatewayServerId)
		{
			removedAccountIds.push_back(iter->first);
			iter = mGameSessions.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	return removedAccountIds;
}

bool GameSessionManager::SetSessionPending(const uint64 accountId)
{
	UniqueLock lock(mLock);

	const auto iter = mGameSessions.find(accountId);
	if (iter == mGameSessions.end())
	{
		return false;
	}

	iter->second.mState = eGameSessionState::Pending;
	iter->second.mPendingStartMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	return true;
}

Vector<uint64> GameSessionManager::GetExpiredPendingSessions(const int64 timeoutMs) const
{
	SharedLock lock(mLock);

	const int64 nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	Vector<uint64> expired;

	for (const auto& [accountId, sessionInfo] : mGameSessions)
	{
		if (sessionInfo.mState == eGameSessionState::Pending && sessionInfo.mPendingStartMs > 0)
		{
			if (nowMs - sessionInfo.mPendingStartMs > timeoutMs)
			{
				expired.push_back(accountId);
			}
		}
	}

	return expired;
}
