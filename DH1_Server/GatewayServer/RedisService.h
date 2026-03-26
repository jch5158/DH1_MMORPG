#pragma once

#include "ActorService.h"

using SetStringCallback = std::function<void(const bool)>;
using GetStringCallback = std::function<void(const std::optional<std::string>&)>;
using DeleteKeyCallback = std::function<void(const bool)>;

struct RedisGatewayInfo
{
	int32 gatewayId;
	std::string ip;
	uint16 port;
	eGatewayStatus status;
	int32 currentSessionCount;
};

struct RedisRealmInfo
{
	int32 realmId;
	std::string realmName;
	int32 currentPlayers;
	int32 maxPlayers;
	int32 status;
};

struct RedisSessionInfo
{
	int32 gatewayId;
	uint64 sessionId;
	int64 loginTimestamp;
};

using GetRealmListCallback = std::function<void(const Vector<RedisRealmInfo>&)>;
using GetRealmInfoCallback = std::function<void(const bool, const RedisRealmInfo&)>;

class RedisService final : public std::enable_shared_from_this<RedisService>
{
public:

	explicit RedisService(const std::string& connectionUri, ActorServiceRef pActorService);
	~RedisService() = default;

	void UpdateGatewayInfo(const RedisGatewayInfo& gatewayInfo);
	RedisActorRef GetRedisActorRef();

	void SetStringAsync(std::string key, std::string value, SetStringCallback callback);
	void SetExpireStringAsync(std::string key, std::string value, const int64 ttlSeconds, SetStringCallback callback);
	void GetStringAsync(std::string key, GetStringCallback callback);
	void GetDelStringAsync(std::string key, GetStringCallback callback);
	void DeleteKeyAsync(std::string key, DeleteKeyCallback callback);

	void GetRealmListAsync(GetRealmListCallback callback);
	void GetRealmInfoAsync(const int32 realmId, GetRealmInfoCallback callback);

	// 세션 관리 (온라인 상태)
	void SetSessionAsync(const uint64 accountId, const RedisSessionInfo& sessionInfo, const int64 ttlSeconds, SetStringCallback callback);
	void CheckAndDeleteSessionAsync(const uint64 accountId, const int32 gatewayId, std::function<void(const bool)> callback);
	void RefreshSessionTTLAsync(const Vector<uint64>& accountIds, const int64 ttlSeconds);

	// 크로스 GW 킥 발행
	void PublishKickAsync(const int32 targetGatewayId, const uint64 accountId);

private:
	uint64 mRedisActorId;
	ActorServiceRef mpActorService;
};
