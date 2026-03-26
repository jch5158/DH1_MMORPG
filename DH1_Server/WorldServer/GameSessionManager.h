#pragma once
#include <StlTypes.h>
#include "GameSessionInfo.h"

class GameSessionManager
{
public:
	explicit GameSessionManager(const int32 maxSessionCount);
	~GameSessionManager() = default;

	[[nodiscard]] int32 GetMaxSessionCount() const;
	[[nodiscard]] int32 GetCurrentSessionCount() const;

	[[nodiscard]] bool AddSession(const GameSessionInfo& sessionInfo);
	[[nodiscard]] bool RemoveSession(const uint64 accountId);
	[[nodiscard]] bool UpdateSession(const GameSessionInfo& sessionInfo);
	[[nodiscard]] bool HasSession(const uint64 accountId) const;
	[[nodiscard]] std::optional<GameSessionInfo> GetSession(const uint64 accountId) const;
	[[nodiscard]] Vector<uint64> RemoveSessionsByGateway(const int32 gatewayServerId);

private:
	const int32 mMaxSessionCount;

	mutable SharedMutex mLock;
	HashMap<uint64, GameSessionInfo> mGameSessions;
};
