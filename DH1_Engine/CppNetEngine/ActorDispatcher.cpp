#include "pch.h"
#include "ActorDispatcher.h"

ActorDispatcher::ActorDispatcher(ActorScheduler& scheduler, ActorManager& actorManager)
	: mScheduler(scheduler)
	, mActorManager(actorManager)
{
}
