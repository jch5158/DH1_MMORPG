// ReSharper disable CppInconsistentNaming
#pragma once

#include "MemoryAllocator.h"

template <typename T>
class SharedPtrAllocator final
{
public:

    using value_type = T;

	explicit SharedPtrAllocator() = default;
	explicit SharedPtrAllocator(const SharedPtrAllocator&) = default;
	explicit SharedPtrAllocator(SharedPtrAllocator&&) = default;
	SharedPtrAllocator& operator=(const SharedPtrAllocator&) = default;
	SharedPtrAllocator& operator=(SharedPtrAllocator&&) = default;
	
	~SharedPtrAllocator() = default;
    
	template <typename U>
	explicit SharedPtrAllocator(const SharedPtrAllocator<U>&) {}

	template<typename U>
	bool operator==(const SharedPtrAllocator<U>&) const { return true; }

	template<typename U>
	bool operator!=(const SharedPtrAllocator<U>&) const { return false; }

    static T* allocate(const uint64 size)
	{
		const uint64 sharedPtrSize = sizeof(T) * size;

        T* ptr = static_cast<T*>(MemoryAllocator::GetInstance().Alloc(sharedPtrSize));

        return ptr;
	}

	static void deallocate(T* ptr, const uint64 size)
	{
        const uint64 objSize = sizeof(T) * size;

        MemoryAllocator::GetInstance().Free(ptr, objSize);
	}
};

class SharedPtrUtils final
{
public:

	SharedPtrUtils() = delete;
	~SharedPtrUtils() = delete;
	SharedPtrUtils(SharedPtrUtils&) = delete;
	SharedPtrUtils& operator=(SharedPtrUtils&) = delete;
	SharedPtrUtils(SharedPtrUtils&&) = delete;
	SharedPtrUtils& operator=(SharedPtrUtils&&) = delete;

	template <typename T, typename... Args>
	static std::shared_ptr<T> Alloc(Args&&... args)
	{
		return std::allocate_shared<T>(SharedPtrAllocator<T>(), std::forward<Args>(args)...);
	}
};

template <typename T>
using WeakPtr = std::weak_ptr<T>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;

#define DECLARE_SHARED_PTR(TypeName) \
	class TypeName; /*NOLINT(bugprone-macro-parentheses)*/ \
	using TypeName##Ref = SharedPtr<TypeName>; \
	using TypeName##ConstRef = SharedPtr<const TypeName>;

#define DECLARE_SMART_PTR(TypeName) \
	DECLARE_SHARED_PTR(TypeName) \
	using TypeName##Weak = WeakPtr<TypeName>; \
	using TypeName##ConstWeak = WeakPtr<const TypeName>;

DECLARE_SMART_PTR(IocpObject);
DECLARE_SMART_PTR(SocketIocpObject)
DECLARE_SMART_PTR(Listener);
DECLARE_SMART_PTR(Acceptor);
DECLARE_SMART_PTR(Session);
DECLARE_SMART_PTR(GameSession);
DECLARE_SMART_PTR(PacketSession);
DECLARE_SMART_PTR(NetSendBuffer);

DECLARE_SMART_PTR(IActor);
DECLARE_SMART_PTR(Actor);
DECLARE_SMART_PTR(ScopedActor);
DECLARE_SMART_PTR(Message);

DECLARE_SMART_PTR(SessionReaper);
DECLARE_SMART_PTR(SessionManager);
DECLARE_SMART_PTR(WaitQueueManager);

DECLARE_SMART_PTR(IocpCore);
DECLARE_SMART_PTR(NetworkScheduler);
DECLARE_SMART_PTR(ActorScheduler);

DECLARE_SMART_PTR(Service);
DECLARE_SMART_PTR(ClientService);
DECLARE_SMART_PTR(ServerService);
