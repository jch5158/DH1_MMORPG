#pragma once
#include "Actor.h"
#include "TimingWheel.h"

class ActorEvent;

class ScopedActor final : public IActor
{
public:

	static constexpr int32 DEFAULT_RETRY_LIMIT = 1000;
	static constexpr int32 DEFAULT_SPIN_LIMIT = 5000;

	template <typename... Args>
	explicit ScopedActor(Args&&... args)
		: IActor()
		, mRetryLimit(DEFAULT_RETRY_LIMIT)
		, mSpinLimit(DEFAULT_SPIN_LIMIT)
		, mAcquireIndex(-1)
		, mActors()
	{
		mActors.reserve(sizeof...(Args));
		(mActors.emplace_back(std::forward<Args>(args)), ...);

		std::sort(mActors.begin(), mActors.end(), [](const ActorRef& pLeft, const ActorRef& pRight)->bool
			{
				return pLeft->GetId() < pRight->GetId();
			});

		mActors.erase(std::unique(mActors.begin(), mActors.end(), [](const ActorRef& pLeft, const ActorRef& pRight) -> bool
			{
				return pLeft->GetId() == pRight->GetId();
			}), mActors.end());
	}

	virtual ~ScopedActor() override = default;

	virtual void Dispatch(class IocpEvent& iocpEvent, const uint32) override;

	virtual bool TryAcquire() override;
	virtual void Release() override;

	virtual void Activate() override;
	virtual void Register() override;
	virtual void Flush() override;
	virtual IocpEvent& GetIocpEvent() override;
	virtual int32 GetMessageCount() override;
	virtual void Post(MessageRef pMessage) override;

	void SetRetryLimit(const int32 retryLimit);
	[[nodiscard]] int32 GetRetryLimit() const;

	void SetSpinLimit(const int32 spinCount);
	[[nodiscard]] int32 GetSpinLimit() const;

private:

	bool tryAcquireAll();

	int32 mRetryLimit;
	int32 mSpinLimit;
	int32 mAcquireIndex;
	Vector<ActorRef> mActors;
	ActorMailBox mMailbox;
};

