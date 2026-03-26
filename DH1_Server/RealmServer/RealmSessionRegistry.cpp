#include "pch.h"
#include "RealmSessionRegistry.h"

RealmSessionRegistry::RealmSessionRegistry(const int32 reserveCount)
{
	mSessions.reserve(reserveCount);
}

bool RealmSessionRegistry::AddSession(const RealmSessionInfo& sessionInfo)
{
	UniqueLock lock(mLock);
	return mSessions.try_emplace(sessionInfo.mAccountId, sessionInfo).second;
}

bool RealmSessionRegistry::RemoveSession(const uint64 accountId)
{
	UniqueLock lock(mLock);
	return mSessions.erase(accountId) > 0;
}

bool RealmSessionRegistry::HasSession(const uint64 accountId) const
{
	SharedLock lock(mLock);
	return mSessions.contains(accountId);
}

std::optional<RealmSessionInfo> RealmSessionRegistry::GetSession(const uint64 accountId) const
{
	SharedLock lock(mLock);

	const auto iter = mSessions.find(accountId);
	if (iter == mSessions.end())
	{
		return std::nullopt;
	}

	return iter->second;
}

Vector<uint64> RealmSessionRegistry::RemoveSessionsByWorldServer(const int32 worldServerId)
{
	UniqueLock lock(mLock);

	Vector<uint64> removedAccountIds;

	for (auto iter = mSessions.begin(); iter != mSessions.end();)
	{
		if (iter->second.mWorldServerId == worldServerId)
		{
			removedAccountIds.push_back(iter->first);
			iter = mSessions.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	return removedAccountIds;
}

int32 RealmSessionRegistry::GetTotalSessionCount() const
{
	SharedLock lock(mLock);
	return static_cast<int32>(mSessions.size());
}

int32 RealmSessionRegistry::GetSessionCountByWorld(const int32 worldServerId) const
{
	SharedLock lock(mLock);

	int32 count = 0;
	for (const auto& [accountId, session] : mSessions)
	{
		if (session.mWorldServerId == worldServerId)
		{
			++count;
		}
	}

	return count;
}
