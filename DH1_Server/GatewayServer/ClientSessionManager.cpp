#include "pch.h"
#include "ClientSessionManager.h"

ClientSessionManager::ClientSessionManager(const int32 maxSessionCount)
	:mMaxSessionCount(maxSessionCount)
{
	mClientSessions.reserve(maxSessionCount);
}

int32 ClientSessionManager::GetMaxSessionCount() const
{
	return mMaxSessionCount;
}

bool ClientSessionManager::AddClientSession(const uint64 accountId, ClientSessionRef pSession)
{
	UniqueLock lock(mLock);
	const bool isSuccess = mClientSessions.try_emplace(accountId, pSession).second;
	return isSuccess;
}

bool ClientSessionManager::RemoveClientSession(const uint64 accountId)
{
	UniqueLock lock(mLock);
	const bool isSuccess = mClientSessions.erase(accountId) > 0;
	return isSuccess;
}

ClientSessionRef ClientSessionManager::GetClientSession(const uint64 accountId)
{
	SharedLock lock(mLock);
	const auto iter = mClientSessions.find(accountId);
	if (iter == mClientSessions.end())
	{
		return nullptr;
	}

	return iter->second;
}
