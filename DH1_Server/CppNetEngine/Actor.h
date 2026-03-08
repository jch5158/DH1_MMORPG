#pragma once
#include "ActorEvent.h"
#include "ActorJobQueue.h"
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

	explicit IActor(ActorSchedulerRef pScheduler);
	virtual ~IActor() = default;

	virtual void Dispatch(ActorEvent& actorEvent) = 0;
	[[nodiscard]] virtual bool TryAcquire() = 0;
	virtual void Release() = 0;
	
	void Activate();
	void Register();
	void Flush();
	[[nodiscard]] bool PushJob(const JobRef& pJob);
	[[nodiscard]] int32 GetJobCount() const;
	[[nodiscard]] ActorSchedulerRef GetActorSchedulerRef() const;

protected:

	ActorSchedulerRef mpScheduler;
	ActorJobQueue mJobQueue;
};

class Actor : public IActor
{
public:

	explicit Actor(const ActorSchedulerRef& pScheduler);
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

	virtual void Dispatch(ActorEvent& actorEvent) override;
	[[nodiscard]] virtual bool TryAcquire() override;
	virtual void Release() override;

	[[nodiscard]] int64 GetSeed() const;

private:
	static std::atomic<int64> sSeedBase;

	const int64 mSeed;
	std::atomic<bool> mbAcquire;
};
