#pragma once
#include "ActorManager.h"
#include "ActorDispatcher.h"

class ActorService : public std::enable_shared_from_this<ActorService>
{
public:
	explicit ActorService(ActorScheduler& actorScheduler);
	~ActorService() = default;

private:
	ActorScheduler& mScheduler;
	ActorManager mActorManager;
	ActorDispatcher mActorDispatcher;
};

