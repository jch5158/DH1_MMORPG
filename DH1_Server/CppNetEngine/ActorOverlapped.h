#pragma once

class ActorOverlapped : public OVERLAPPED
{
public:

	ActorOverlapped(const ActorOverlapped&) = default;
	ActorOverlapped& operator=(const ActorOverlapped&) = default;
	ActorOverlapped(ActorOverlapped&&) = default;
	ActorOverlapped& operator=(ActorOverlapped&&) = default;

	explicit ActorOverlapped();
	~ActorOverlapped();

	IActorRef GetOwner();
	void SetOwner(const IActorRef& pOwner);
	void ResetOwner();

	void Clear();
	void ClearOverlapped();

private:

	IActorRef mpOwner;
};
