#include "pch.h"
#include "ActorService.h"
#include "Actor.h"

ActorService::ActorService()
	: mbInitialize(false)
	, mpActorScheduler()
	, mActorManager()
	, mpActorDispatcher()
{
}

bool ActorService::Initialize(const ActorServiceConfig& config)
{
	if (mbInitialize.exchange(true) == true)
	{
		return false;
	}

	mpActorScheduler = cpp_net_engine::MakeShared<ActorScheduler>(config.schedulerConfig);
	mpActorDispatcher = std::make_unique<ActorDispatcher>(*mpActorScheduler, mActorManager);

	return true;
}

bool ActorService::Start()
{
	if (!IsInitialized())
	{
		return false;
	}

	return true;
}

void ActorService::CloseService()
{
}

void ActorService::Dispatch() const
{
	if (mpActorScheduler == nullptr)
	{
		return;
	}

	mpActorScheduler->Dispatch();
}

bool ActorService::RegisterActor(const ActorRef& pActor)
{
	if (pActor == nullptr)
	{
		return false;
	}

	if (pActor->Activate(mpActorScheduler) == false)
	{
		return false;
	}

	if (mActorManager.AddActorRef(pActor) == false)
	{
		return false;
	}

	return true;
}

bool ActorService::UnregisterActor(const uint64 actorId)
{
	return mActorManager.RemoveActorRef(actorId);
}

ActorRef ActorService::GetActorRef(const uint64 actorId)
{
	return mActorManager.GetActorRef(actorId);
}

ActorDispatcher& ActorService::GetActorDispatcher()
{
	return *mpActorDispatcher;
}

ActorSchedulerRef ActorService::GetActorScheduler() const
{
	return mpActorScheduler;
}

bool ActorService::IsInitialized() const
{
	return mbInitialize.load();
}

uint64 ActorService::GetActorCount()
{
	return mActorManager.GetActorCount();
}
