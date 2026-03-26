#pragma once
#include <sw/redis++/redis++.h>

class RedisSubscriber
{
public:

	RedisSubscriber() = default;
	~RedisSubscriber() = default;

	RedisSubscriber(const RedisSubscriber&) = delete;
	RedisSubscriber& operator=(const RedisSubscriber&) = delete;
	RedisSubscriber(RedisSubscriber&&) = delete;
	RedisSubscriber& operator=(RedisSubscriber&&) = delete;

	void Start(const std::string& connectionUri, const int32 gatewayId);
	void Stop();

	[[nodiscard]] bool IsRunning() const { return mbRunning.load(); }

private:

	void subscriberLoop();
	void onKickMessage(const std::string& message);

	std::string mConnectionUri;
	int32 mGatewayId = 0;
	std::atomic<bool> mbRunning{ false };
	std::unique_ptr<sw::redis::Redis> mRedis;
};
