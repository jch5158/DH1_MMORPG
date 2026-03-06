#include "pch.h"
#include "PlayerManager.h"

bool PlayerManager::AddPlayer(const PacketSessionRef& pSession, const PlayerRef& pPlayer)
{
	if (pSession == nullptr || pPlayer == nullptr)
	{
		return false;
	}

	UniqueLock lock(mLock);
	return mPlayers.insert(std::pair(pSession, pPlayer)).second;
}

PlayerRef PlayerManager::FindPlayer(const PacketSessionRef& pSession)
{
	if (pSession == nullptr)
	{
		return nullptr;
	}

	UniqueLock lock(mLock);

	const auto iter = mPlayers.find(pSession);
	if (iter == mPlayers.end())
	{
		return nullptr;
	}

	return iter->second;
}

void PlayerManager::RemovePlayer(const PacketSessionRef& pSession)
{
	if (pSession == nullptr)
	{
		return;
	}

	mPlayers.erase(pSession);
}

PlayerManager::PlayerManager()
	: mLock()
	, mPlayers()
{
}
