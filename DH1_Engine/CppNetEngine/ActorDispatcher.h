// ReSharper disable CppClangTidyModernizeUseNodiscard
#pragma once
#include "Actor.h"
#include "ActorManager.h"

class ActorDispatcher
{
public:
	using CallbackType = std::function<void()>;

	explicit ActorDispatcher(ActorScheduler& scheduler, ActorManager& actorManager);
	~ActorDispatcher() = default;

	void Post(const uint64 actorId, CallbackType&& callback) const
	{
		const IActorRef pActor = mActorManager.GetActorRef(actorId);
		if (pActor == nullptr)
		{
			return;
		}

		const auto pMessage = cpp_net_engine::MakeShared<Message>(std::move(callback));
		pActor->Post(pMessage);
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	void Post(const uint64 actorId, Ret(T::* pMemFunc)(FuncArgs...), CallArgs&&... args) const
	{
		const IActorRef pActor = mActorManager.GetActorRef(actorId);
		if (pActor == nullptr)
		{
			return;
		}

		const auto pOwner = std::static_pointer_cast<T>(pActor);
		const auto pMessage = cpp_net_engine::MakeShared<Message>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		pOwner->Post(pMessage);
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	void Post(const uint64 actorId, Ret(T::* pMemFunc)(FuncArgs...) const, CallArgs&&... args) const
	{
		const IActorRef pActor = mActorManager.GetActorRef(actorId);
		if (pActor == nullptr)
		{
			return;
		}

		const auto pOwner = std::static_pointer_cast<T>(pActor);
		const auto pMessage = cpp_net_engine::MakeShared<Message>(pOwner, pMemFunc, std::forward<CallArgs>(args)...);
		pOwner->Post(pMessage);
	}

	TimerHandle PostDelay(const uint64 actorId, const int64 delayMs, CallbackType&& callback) const
	{
		const IActorRef pActor = mActorManager.GetActorRef(actorId);
		if (pActor == nullptr)
		{
			return TimerHandle();
		}

		const IActorWeak pActorWeak = pActor;
		TimerHandle handle = mScheduler.RegisterDelay([pActorWeak, capCallback = std::move(callback)]() mutable -> void
			{
				const IActorRef pLockedActor = pActorWeak.lock();
				if (pLockedActor == nullptr)
				{
					return;
				}

				const auto pMessage = cpp_net_engine::MakeShared<Message>(std::move(capCallback));
				pLockedActor->Post(pMessage);
			}, delayMs);
	
		return handle;
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	TimerHandle PostDelay(const uint64 actorId, const int64 delayMs, Ret(T::* pMemFunc)(FuncArgs...), CallArgs&&... args) const
	{
		const IActorRef pActor = mActorManager.GetActorRef(actorId);
		if (pActor == nullptr)
		{
			return TimerHandle();
		}

		const IActorWeak pActorWeak = pActor;
		TimerHandle handle = mScheduler.RegisterDelay([pActorWeak, pMemFunc, ...args = std::forward<CallArgs>(args)]() mutable -> void
			{
				const IActorRef pLockedActor = pActorWeak.lock();
				if (pLockedActor == nullptr)
				{
					return;
				}

				const auto pOwner = std::static_pointer_cast<T>(pLockedActor);
				const auto pMessage = cpp_net_engine::MakeShared<Message>(pOwner, pMemFunc, std::move(args)...);
				pLockedActor->Post(pMessage);
			}, delayMs);

		return handle;
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	TimerHandle PostDelay(const uint64 actorId, const int64 delayMs, Ret(T::* pMemFunc)(FuncArgs...) const, CallArgs&&... args) const
	{
		const IActorRef pActor = mActorManager.GetActorRef(actorId);
		if (pActor == nullptr)
		{
			return TimerHandle();
		}

		const IActorWeak pActorWeak = pActor;
		TimerHandle handle = mScheduler.RegisterDelay([pActorWeak, pMemFunc, ...args = std::forward<CallArgs>(args)]() mutable -> void
			{
				const IActorRef pLockedActor = pActorWeak.lock();
				if (pLockedActor == nullptr)
				{
					return;
				}

				const auto pOwner = std::static_pointer_cast<T>(pLockedActor);
				const auto pMessage = cpp_net_engine::MakeShared<Message>(pOwner, pMemFunc, std::move(args)...);
				pLockedActor->Post(pMessage);
			}, delayMs);

		return handle;
	}

	static void Post(const IActorRef& pActor, CallbackType&& callback)
	{
		if (pActor == nullptr)
		{
			return;
		}

		const auto pMessage = cpp_net_engine::MakeShared<Message>(std::move(callback));
		pActor->Post(pMessage);
	}

	TimerHandle PostDelay(const IActorRef& pActor, const int64 delayMs, CallbackType&& callback) const
	{
		if (pActor == nullptr)
		{
			return TimerHandle();
		}

		const IActorWeak pActorWeak = pActor;
		TimerHandle handle = mScheduler.RegisterDelay([pActorWeak, capCallback = std::move(callback)]() mutable -> void
			{
				const IActorRef pLockedActor = pActorWeak.lock();
				if (pLockedActor == nullptr)
				{
					return;
				}

				const auto pMessage = cpp_net_engine::MakeShared<Message>(std::move(capCallback));
				pLockedActor->Post(pMessage);
			}, delayMs);

		return handle;
	}

private:
	ActorScheduler& mScheduler;
	ActorManager& mActorManager;
};

