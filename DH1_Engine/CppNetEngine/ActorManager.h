#pragma once
class ActorManager
{
public:
	ActorManager();
	~ActorManager() = default;

	[[nodiscard]] ActorRef GetActorRef(const uint64 actorId);
	[[nodiscard]] bool SetActorRef(ActorRef pActor);
	[[nodiscard]] uint64 GetActorCount();

private:

	Mutex mLock;
	HashMap<uint64, ActorRef> mActorMap;
};

