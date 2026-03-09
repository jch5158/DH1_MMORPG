#include "pch.h"
#include "PlayerManager.h"

bool PlayerManager::AddPlayer(PacketSessionRef pSession, PlayerRef pPlayer)
{
	if (pSession == nullptr || pPlayer == nullptr)
	{
		return false;
	}

	UniqueLock lock(mLock);
	return mPlayers.emplace(std::move(pSession), std::move(pPlayer)).second;
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
