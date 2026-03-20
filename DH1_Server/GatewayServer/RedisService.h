#pragma once

#include "ActorDispatcher.h"
#include "ActorScheduler.h"

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
	explicit RedisService(const std::string& connectionUri, ActorSchedulerRef pScheduler);
	~RedisService() = default;

	void UpdateGatewayInfo(const RedisGatewayInfo& gatewayInfo);
	RedisActorRef GetRedisActorRef();

private:
	uint64 mRedisActorId;
	ActorManager mActorManager;
	ActorSchedulerRef mpRedisScheduler;
	ActorDispatcher mActorDispatcher;
};

