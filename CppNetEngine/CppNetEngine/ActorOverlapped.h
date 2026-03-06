#pragma once

class ActorOverlapped : public OVERLAPPED
{
public:

	ActorOverlapped(const ActorOverlapped&) = delete;
	ActorOverlapped operator=(const ActorOverlapped&) = delete;
	ActorOverlapped(ActorOverlapped&&) = delete;
	ActorOverlapped operator=(ActorOverlapped&&) = delete;

	ActorOverlapped();
	~ActorOverlapped();

	IActorRef GetOwner();
	void SetOwner(const IActorRef& pOwner);
	void ResetOwner();

	void Clear();
	void ClearOverlapped();

private:

	IActorRef mpOwner;
};

