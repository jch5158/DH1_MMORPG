#pragma once

#include "ActorService.h"

struct RedisRealmRegistration
{
	int32 realmId;
	std::string realmName;
	int32 currentPlayers;
	int32 maxPlayers;
	int32 status;
};

class RedisService final : public std::enable_shared_from_this<RedisService>
{
public:

	explicit RedisService(const std::string& connectionUri, ActorServiceRef pActorService);
	~RedisService() = default;

	void UpdateRealmRegistration(const RedisRealmRegistration& registration, const int64 ttlSeconds);

private:
	uint64 mRedisActorId;
	ActorServiceRef mpActorService;
};

using RedisServiceRef = std::shared_ptr<RedisService>;
