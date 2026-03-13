#pragma once
#include "IocpEvent.h"

class ActorMessageEvent final : public IocpEvent
{
public:
	explicit ActorMessageEvent();
};

class ActorMailBox final
{
public:

	static constexpr int32 DEFAULT_EXECUTE_MSG_COUNT = 50;

	explicit ActorMailBox(const int32 executeMsgCount = DEFAULT_EXECUTE_MSG_COUNT);
	~ActorMailBox() = default;

	void SetOwner(const IocpObjectRef& pOwner);
	ActorMessageEvent& GetActorMessageEvent();

	void Post(MessageRef&& pMessage);

	[[nodiscard]] int32 GetMessageCount() const;
	void Process();
	void Register();
	void Flush();

private:
	ActorMessageEvent mActorMessageEvent;
	const int32 mMaxExecuteMsgCount;
	LockFreeQueue<MessageRef> mMailbox;
};