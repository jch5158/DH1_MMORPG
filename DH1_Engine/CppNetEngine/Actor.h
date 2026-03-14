#pragma once
#include "ActorMailbox.h"
#include "LockFreeQueue.h"
#include "Message.h"
#include "ActorScheduler.h"
#include "IocpCore.h"

class IActor : public IocpObject
{
public:
	IActor(const IActor&) = delete;
	IActor& operator=(const IActor&) = delete;
	IActor(IActor&&) = delete;
	IActor& operator=(IActor&&) = delete;

	explicit IActor();
	virtual ~IActor() override = default;

	virtual void Dispatch(class IocpEvent& iocpEvent, const uint32) override = 0;

	virtual bool TryAcquire() = 0;
	virtual void Release() = 0;

	virtual bool Activate(ActorScheduler& scheduler) = 0;
	virtual void Flush() = 0;
	virtual IocpEvent& GetIocpEvent() = 0;
	virtual int32 GetMessageCount()  = 0;
	virtual void Post(MessageRef pMessage) = 0;
	[[nodiscard]] uint64 GetId() const;

private:
	static std::atomic<uint64> sSeedBase;
	const uint64 mId;
};

class Actor : public IActor
{
public:

	explicit Actor();
	virtual ~Actor() override = default;

	virtual void Dispatch(class IocpEvent& iocpEvent, const uint32) override;
	virtual bool TryAcquire() override;
	virtual void Release() override;

	virtual bool Activate(ActorScheduler& scheduler) override;
	virtual void Flush() override;
	virtual IocpEvent& GetIocpEvent() override;
	virtual int32 GetMessageCount() override;
	virtual void Post(MessageRef pMessage) override;

private:

	void processActorMessage();

	std::atomic<bool> mbAcquire;
	ActorMailbox mMailbox;
};
