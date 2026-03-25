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

struct RedisWorldServerInfo
{
	int32 worldId;
	std::string worldName;
	int32 currentPlayers;
	int32 maxPlayers;
	int32 status;
};

using GetWorldServerListCallback = std::function<void(const Vector<RedisWorldServerInfo>&)>;
using GetWorldServerInfoCallback = std::function<void(const bool, const RedisWorldServerInfo&)>;

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

	void GetWorldServerListAsync(GetWorldServerListCallback callback);
	void GetWorldServerInfoAsync(const int32 worldId, GetWorldServerInfoCallback callback);

private:
	uint64 mRedisActorId;
	ActorServiceRef mpActorService;
};
