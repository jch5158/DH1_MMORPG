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
	void DeleteKeyAsync(std::string key, DeleteKeyCallback callback);

private:
	uint64 mRedisActorId;
	ActorServiceRef mpActorService;
};
