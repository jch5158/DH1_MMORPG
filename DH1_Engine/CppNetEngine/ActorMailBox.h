#pragma once
#include "IocpEvent.h"

class ActorScheduler;

class ActorMessageEvent final : public IocpEvent
{
public:
	explicit ActorMessageEvent();
};

class ActorMailbox final
{
public:

	static constexpr int32 MIN_EXECUTE_MSG_COUNT = 16;
	static constexpr int32 DEFAULT_EXECUTE_MSG_COUNT = 64;
	static constexpr int32 MAX_EXECUTE_MSG_COUNT = 256;

	explicit ActorMailbox();
	~ActorMailbox() = default;

	bool Initialize(const IocpObjectRef& pOwner, ActorScheduler& scheduler);

	ActorMessageEvent& GetActorMessageEvent();
	void SetExecuteMsgCount(const int32 executeMsgCount);

	void Post(MessageRef&& pMessage);

	[[nodiscard]] int32 GetMessageCount() const;
	void Process();
	void Register();
	void Flush();

private:
	ActorMessageEvent mActorMessageEvent;
	int32 mMaxExecuteMsgCount;
	ActorScheduler* mpScheduler;
	LockFreeQueue<MessageRef> mMailbox;
};