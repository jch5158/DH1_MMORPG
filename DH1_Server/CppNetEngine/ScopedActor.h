#pragma once

class ScopedActor final : public IActor
{
public:

	static constexpr int32 DEFAULT_SPIN_COUNT = 5000;

	template <typename... Args>
	explicit ScopedActor(ActorContext actorContext, Args&&... args)
		: IActor(std::move(actorContext))
		, mSpinCount(DEFAULT_SPIN_COUNT)
		, mAcquireIndex(-1)
		, mActors()
		, mJobQueue()
	{
		mActors.reserve(sizeof...(Args));
		(mActors.emplace_back(std::forward<Args>(args)), ...);

		mActors.erase(std::unique(mActors.begin(), mActors.end(), [](const ActorRef& pLeft, const ActorRef& pRight) -> bool
			{
				return pLeft->GetSeed() == pRight->GetSeed();
			}), mActors.end());

		std::sort(mActors.begin(), mActors.end(), [](const ActorRef& pLeft, const ActorRef& pRight)->bool
			{
				return pLeft->GetSeed() < pRight->GetSeed();
			});
	}

	virtual ~ScopedActor() override = default;

	virtual void Execute() override;
	[[nodiscard]] virtual bool TryAcquire() override;
	virtual void Release() override;
	virtual void Register() override;
	virtual void Flush() override;
	[[nodiscard]] virtual bool PushJob(const JobRef& pJob) override;
	[[nodiscard]] virtual int32 GetJobCount() override;

	void Post( CallbackType&& callback);
	TimerHandle PostDelay(CallbackType&& callback, const int64 delayMs);

	void SetSpinCount(const int32 spinCount);
	[[nodiscard]] int32 GetSpinCount() const;

private:

	bool tryAcquireAll();

	int32 mSpinCount;
	int32 mAcquireIndex;
	Vector<ActorRef> mActors;
	LockFreeQueue<JobRef> mJobQueue;
};

