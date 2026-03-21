#pragma once
#include "Types.h"
#include "SharedPtrUtils.h"

enum class eIocpEventType : uint8
{
	Accept = 1,
	Connect,
	Disconnect,
	Send,
	Receive,
	CustomEvent
};

class IocpEvent : public OVERLAPPED
{
public:

	IocpEvent(const IocpEvent&) = delete;
	IocpEvent& operator=(const IocpEvent&) = delete;
	IocpEvent(IocpEvent&&) = delete;
	IocpEvent& operator=(IocpEvent&&) = delete;

	explicit IocpEvent(const eIocpEventType eventType);
	explicit IocpEvent(const int32 customType);
	~IocpEvent() = default;

	void ClearOverlapped();
	[[nodiscard]] eIocpEventType GetEventType() const;
	[[nodiscard]] IocpObjectRef GetOwner() const;
	[[nodiscard]] int32 GetCustomType() const;

	void SetOwner(const IocpObjectRef& pOwner);
	void ResetOwner();

private:

	const eIocpEventType mEventType;
	const std::optional<int32> mCustomType;
	IocpObjectWeak mpOwner;
};