#pragma once
#include "ActorContext.h"
#include "ActorOverlapped.h"
#include "LockFreeQueue.h"
#include "Job.h"
#include "JobDispatcher.h"
#include "ActorScheduler.h"

class IActor : public std::enable_shared_from_this<IActor>
{
public:

	using CallbackType = std::function<void()>;

	IActor(const IActor&) = delete;
	IActor& operator=(const IActor&) = delete;
	IActor(IActor&&) = delete;
	IActor& operator=(IActor&&) = delete;

	explicit IActor(ActorContext actorContext);
	virtual ~IActor() = default;

	virtual void Execute() = 0;
	[[nodiscard]] virtual bool TryAcquire() = 0;
	virtual void Release() = 0;
	virtual void Register() = 0;
	virtual void Flush() = 0;
	[[nodiscard]] virtual bool PushJob(const JobRef& pJob) = 0;
	[[nodiscard]] virtual int32 GetJobCount() = 0;

	ActorOverlapped& GetActorOverlapped();
	void ClearActorOverlapped();

	ActorSchedulerRef GetActorSchedulerRef() const;

private:

	ActorContext mActorContext;
};

class Actor : public IActor
{
public:

	explicit Actor(ActorContext actorContext);
	virtual ~Actor() override = default;

	void Post(CallbackType&& callback)
	{
		const auto pJob = cpp_net_engine::MakeShared<Job>(std::move(callback));
		JobDispatcher::Post(pJob, shared_from_this());
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	void Post(Ret(T::* pMemFunc)(FuncArgs...), CallArgs&&... args)
	{
		const auto pOwner = std::static_pointer_cast<T>(shared_from_this());
		const auto pJob = cpp_net_engine::MakeShared<Job>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		JobDispatcher::Post(pJob, pOwner);
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	void Post(Ret(T::* pMemFunc)(FuncArgs...) const, CallArgs&&... args)
	{
		const auto pOwner = std::static_pointer_cast<T>(shared_from_this());
		const auto pJob = cpp_net_engine::MakeShared<Job>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		JobDispatcher::Post(pJob, pOwner);
	}

	TimerHandle PostDelay(const int64 delayMs, CallbackType&& callback)
	{
		const auto pJob = cpp_net_engine::MakeShared<Job>(std::move(callback));
		TimerHandle handle = JobDispatcher::PostDelay(pJob, shared_from_this(), GetActorSchedulerRef(), delayMs);
		return handle;
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	TimerHandle PostDelay(const int64 delayMs, Ret(T::* pMemFunc)(FuncArgs...), CallArgs&&... args)
	{
		const auto pOwner = std::static_pointer_cast<T>(shared_from_this());
		const auto pJob = cpp_net_engine::MakeShared<Job>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		TimerHandle handle = JobDispatcher::PostDelay(pJob, pOwner, GetActorSchedulerRef(), delayMs);
		return handle;
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	TimerHandle PostDelay(const int64 delayMs, Ret(T::* pMemFunc)(FuncArgs...) const, CallArgs&&... args)
	{
		const auto pOwner = std::static_pointer_cast<T>(shared_from_this());
		const auto pJob = cpp_net_engine::MakeShared<Job>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		TimerHandle handle = JobDispatcher::PostDelay(pJob, pOwner, GetActorSchedulerRef(), delayMs);
		return handle;
	}

	virtual void Execute() override;
	[[nodiscard]] virtual bool TryAcquire() override;
	virtual void Release() override;
	virtual void Register() override;
	virtual void Flush() override;
	[[nodiscard]] virtual bool PushJob(const JobRef& pJob) override;
	[[nodiscard]] virtual int32 GetJobCount() override;


	void Clear();
	[[nodiscard]] int32 Count() const;
	[[nodiscard]] int64 GetSeed() const;

private:
	static std::atomic<int64> sSeedBase;

	const int64 mSeed;
	std::atomic<bool> mbAcquire;
	LockFreeQueue<JobRef> mJobQueue;
};
