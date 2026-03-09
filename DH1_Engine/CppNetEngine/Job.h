#pragma once

class Job
{
public:
	using CallbackType = std::function<void()>;

	explicit Job(CallbackType&& func)
		: mJobFunc(std::move(func))
	{
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	Job(std::shared_ptr<T> pOwner, Ret(T::* pMemFunc)(FuncArgs...), CallArgs&&... args)
	{
		mJobFunc = [pOwner, pMemFunc, ...capArgs = std::forward<CallArgs>(args)]() mutable -> void
			{
				std::invoke(pMemFunc, pOwner, std::forward<CallArgs>(capArgs)...);
			};
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	Job(std::shared_ptr<T> pOwner, Ret(T::* pMemFunc)(FuncArgs...) const, CallArgs&&... args)
	{
		mJobFunc = [pOwner, pMemFunc, ...capArgs = std::forward<CallArgs>(args)]() mutable -> void
			{
				std::invoke(pMemFunc, pOwner, std::forward<CallArgs>(capArgs)...);
			};
	}

	void Execute() const
	{
		if (mJobFunc != nullptr)
		{
			mJobFunc();
		}
	}

private:

	CallbackType mJobFunc;
};

