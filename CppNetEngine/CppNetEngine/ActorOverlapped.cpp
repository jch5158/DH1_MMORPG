#include "pch.h"
#include "ActorOverlapped.h"

ActorOverlapped::ActorOverlapped()
	: OVERLAPPED{}
	, mpOwner()
{
}

ActorOverlapped::~ActorOverlapped()
{
	Clear();
}

IActorRef ActorOverlapped::GetOwner()
{
	return mpOwner;
}

void ActorOverlapped::SetOwner(const IActorRef& pOwner)
{
	mpOwner = pOwner;
}

void ActorOverlapped::ResetOwner()
{
	mpOwner.reset();
}

void ActorOverlapped::Clear()
{
	ClearOverlapped();
	ResetOwner();
}

void ActorOverlapped::ClearOverlapped()
{
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
	OVERLAPPED::hEvent = nullptr;
}
