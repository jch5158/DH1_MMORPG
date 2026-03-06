#pragma once
#include "ActorOverlapped.h"
#include "LockFreeQueue.h"
#include "Job.h"
#include "JobDispatcher.h"
#include "ActorScheduler.h"

class IActor : public std::enable_shared_from_this<IActor>
{
public:

	using CallbackType = std::function<void()>;

	explicit IActor();
	virtual ~IActor() = default;

	virtual void Execute() = 0;
	[[nodiscard]] virtual bool TryAcquire() = 0;
	virtual void Release() = 0;
	virtual void Register(const ActorSchedulerRef& pActorScheduler) = 0;
	virtual void Flush() = 0;
	[[nodiscard]] virtual bool PushJob(const JobRef& pJob) = 0;
	[[nodiscard]] virtual int32 GetJobCount() = 0;

	ActorOverlapped& GetActorOverlapped();
	void ClearActorOverlapped();

private:

	ActorOverlapped mActorOverlapped;
};

class Actor : public IActor
{
public:

	explicit Actor();
	virtual ~Actor() override = default;

	void Post(const ActorSchedulerRef& pScheduler, CallbackType&& callback)
	{
		const auto pJob = cpp_net_engine::MakeShared<Job>(std::move(callback));
		JobDispatcher::Post(pJob, shared_from_this(), pScheduler);
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	void Post(const ActorSchedulerRef& pScheduler, Ret(T::* pMemFunc)(FuncArgs...), CallArgs&&... args)
	{
		const auto pOwner = std::static_pointer_cast<T>(shared_from_this());
		const auto pJob = cpp_net_engine::MakeShared<Job>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		JobDispatcher::Post(pJob, pOwner, pScheduler);
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	void Post(const ActorSchedulerRef& pScheduler, Ret(T::* pMemFunc)(FuncArgs...) const, CallArgs&&... args)
	{
		const auto pOwner = std::static_pointer_cast<T>(shared_from_this());
		const auto pJob = cpp_net_engine::MakeShared<Job>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		JobDispatcher::Post(pJob, pOwner, pScheduler);
	}

	TimerHandle PostDelay(const ActorSchedulerRef& pScheduler, const int64 delayMs, CallbackType&& callback)
	{
		const auto pJob = cpp_net_engine::MakeShared<Job>(std::move(callback));
		TimerHandle handle = JobDispatcher::PostDelay(pJob, shared_from_this(), pScheduler, delayMs);
		return handle;
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	TimerHandle PostDelay(const ActorSchedulerRef& pScheduler, const int64 delayMs, Ret(T::* pMemFunc)(FuncArgs...), CallArgs&&... args)
	{
		const auto pOwner = std::static_pointer_cast<T>(shared_from_this());
		const auto pJob = cpp_net_engine::MakeShared<Job>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		TimerHandle handle = JobDispatcher::PostDelay(pJob, pOwner, pScheduler, delayMs);
		return handle;
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	TimerHandle PostDelay(const ActorSchedulerRef& pScheduler, const int64 delayMs, Ret(T::* pMemFunc)(FuncArgs...) const, CallArgs&&... args)
	{
		const auto pOwner = std::static_pointer_cast<T>(shared_from_this());
		const auto pJob = cpp_net_engine::MakeShared<Job>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		TimerHandle handle = JobDispatcher::PostDelay(pJob, pOwner, pScheduler, delayMs);
		return handle;
	}

	virtual void Execute() override;
	[[nodiscard]] virtual bool TryAcquire() override;
	virtual void Release() override;
	virtual void Register(const ActorSchedulerRef& pActorScheduler) override;
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
