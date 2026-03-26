#pragma once
#include <StlTypes.h>

struct RealmSessionInfo
{
	uint64 mAccountId = 0;
	int32 mWorldServerId = 0;
	int32 mGatewayServerId = 0;
};

class RealmSessionRegistry
{
public:
	explicit RealmSessionRegistry(const int32 reserveCount);
	~RealmSessionRegistry() = default;

	[[nodiscard]] bool AddSession(const RealmSessionInfo& sessionInfo);
	[[nodiscard]] bool RemoveSession(const uint64 accountId);
	[[nodiscard]] bool HasSession(const uint64 accountId) const;
	[[nodiscard]] std::optional<RealmSessionInfo> GetSession(const uint64 accountId) const;
	[[nodiscard]] Vector<uint64> RemoveSessionsByWorldServer(const int32 worldServerId);
	[[nodiscard]] int32 GetTotalSessionCount() const;
	[[nodiscard]] int32 GetSessionCountByWorld(const int32 worldServerId) const;

private:
	mutable SharedMutex mLock;
	HashMap<uint64, RealmSessionInfo> mSessions;
};
