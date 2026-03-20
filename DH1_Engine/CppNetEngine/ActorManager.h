#pragma once
class ActorManager
{
public:
	ActorManager();
	~ActorManager() = default;

	[[nodiscard]] ActorRef GetActorRef(const uint64 actorId);
	[[nodiscard]] uint64 GetActorCount();
	[[nodiscard]] bool AddActorRef(ActorRef pActor);
	[[nodiscard]] bool RemoveActorRef(const uint64 actorId);
private:

	SharedMutex mLock;
	HashMap<uint64, ActorRef> mActorMap;
};

