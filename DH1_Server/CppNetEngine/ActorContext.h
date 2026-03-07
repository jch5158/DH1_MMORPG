#pragma once
#include "ActorOverlapped.h"

class ActorContext
{
public:
	ActorContext(const ActorContext&) = default;
	ActorContext& operator=(const ActorContext&) = default;
	ActorContext(ActorContext&&) = default;
	ActorContext& operator=(ActorContext&&) = default;

	explicit ActorContext(const ActorSchedulerRef& pScheduler);
	~ActorContext();

	IActorRef GetOwner();
	void SetOwner(const IActorRef& pOwner);
	void ResetOwner();

	void Clear();
	void ClearOverlapped();

	void SetActorScheduler(const ActorSchedulerRef& pScheduler);
	void ResetActorSchedulerRef();
	[[nodiscard]] ActorSchedulerRef GetActorSchedulerRef() const;

public:

	ActorOverlapped mActorOverlapped;
	ActorSchedulerWeak mpSchedulerWeak;
};
