#include "pch.h"
#include "ActorService.h"

ActorService::ActorService(ActorScheduler& actorScheduler)
	: mScheduler(actorScheduler)
	, mActorManager()
	, mActorDispatcher(actorScheduler, mActorManager)
{
}