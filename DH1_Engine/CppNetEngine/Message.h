#pragma once

class Message
{
public:
	using MessageType = std::function<void()>;

	explicit Message(MessageType&& message);

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	Message(std::shared_ptr<T> pOwner, Ret(T::* pMemFunc)(FuncArgs...), CallArgs&&... args)
	{
		mMessage = [pOwner, pMemFunc, ...capArgs = std::forward<CallArgs>(args)]() mutable -> void
			{
				std::invoke(pMemFunc, pOwner, std::forward<CallArgs>(capArgs)...);
			};
	}

	template<typename T, typename Ret, typename... FuncArgs, typename... CallArgs>
	Message(std::shared_ptr<T> pOwner, Ret(T::* pMemFunc)(FuncArgs...) const, CallArgs&&... args)
	{
		mMessage = [pOwner, pMemFunc, ...capArgs = std::forward<CallArgs>(args)]() mutable -> void
			{
				std::invoke(pMemFunc, pOwner, std::forward<CallArgs>(capArgs)...);
			};
	}

	void Execute() const;

private:

	MessageType mMessage;
};
