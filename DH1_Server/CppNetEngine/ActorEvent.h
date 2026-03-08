#pragma once

enum class eActorEventType : uint8
{
	Job
};

class ActorEvent : public OVERLAPPED
{
public:

	ActorEvent(const ActorEvent&) = delete;
	ActorEvent& operator=(const ActorEvent&) = delete;
	ActorEvent(ActorEvent&&) = delete;
	ActorEvent& operator=(ActorEvent&&) = delete;

	explicit ActorEvent(const eActorEventType eventType);
	~ActorEvent() = default;

	[[nodiscard]] IActorRef GetOwner() const;
	void SetOwner(const IActorRef& pOwner);

	[[nodiscard]] eActorEventType GetEventType() const;

private:

	const eActorEventType mEventType;
	IActorWeak mpOwnerWeak;
};

class JobActorEvent : public ActorEvent
{
public:
	JobActorEvent();
	~JobActorEvent() = default;
};