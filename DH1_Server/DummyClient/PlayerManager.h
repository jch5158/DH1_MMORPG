#pragma once
#include "ISingleton.h"
#include "Types.h"

class PlayerManager final : public ISingleton<PlayerManager>
{
public:

	friend class ISingleton<PlayerManager>;

	~PlayerManager() = default;

	[[nodiscard]] bool AddPlayer(const PacketSessionRef& pSession, const PlayerRef& pPlayer);
	[[nodiscard]] PlayerRef FindPlayer(const PacketSessionRef& pSession);
	void RemovePlayer(const PacketSessionRef& pSession);



private:

	explicit PlayerManager();
	
	Mutex mLock;
	HashMap<PacketSessionRef, PlayerRef> mPlayers;
};

