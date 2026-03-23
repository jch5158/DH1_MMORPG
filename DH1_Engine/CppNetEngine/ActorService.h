#pragma once

#include "ActorScheduler.h"
#include "ActorManager.h"
#include "ActorDispatcher.h"

struct ActorServiceConfig
{
	ActorSchedulerConfig schedulerConfig;
};

class ActorService : public std::enable_shared_from_this<ActorService>
{
public:

	ActorService(const ActorService&) = delete;
	ActorService& operator=(const ActorService&) = delete;
	ActorService(ActorService&&) = delete;
	ActorService& operator=(ActorService&&) = delete;

	ActorService();
	virtual ~ActorService() = default;

	bool Initialize(const ActorServiceConfig& config);

	virtual bool Start();
	virtual void CloseService();
	void Dispatch() const;

	bool RegisterActor(const ActorRef& pActor);
	bool UnregisterActor(const uint64 actorId);
	[[nodiscard]] ActorRef GetActorRef(const uint64 actorId);

	[[nodiscard]] ActorDispatcher& GetActorDispatcher();
	[[nodiscard]] ActorSchedulerRef GetActorScheduler() const;

	[[nodiscard]] bool IsInitialized() const;
	[[nodiscard]] uint64 GetActorCount();

private:
	std::atomic<bool> mbInitialize;
	ActorSchedulerRef mpActorScheduler;
	ActorManager mActorManager;
	std::unique_ptr<ActorDispatcher> mpActorDispatcher;
};
