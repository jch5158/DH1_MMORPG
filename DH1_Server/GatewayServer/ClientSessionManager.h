#pragma once

class ClientSessionManager
{
public:
	explicit ClientSessionManager(const int32 maxSessionCount);
	~ClientSessionManager() = default;

	[[nodiscard]] int32 GetMaxSessionCount() const;
	 
	[[nodiscard]] bool AddClientSession(const uint64 accountId, ClientSessionRef pSession);
	[[nodiscard]] bool RemoveClientSession(const uint64 accountId);
	[[nodiscard]] ClientSessionRef GetClientSession(const uint64 accountId);

private:
	const int32 mMaxSessionCount;

	SharedMutex mLock;
	HashMap<uint64, ClientSessionRef> mClientSessions;
};

