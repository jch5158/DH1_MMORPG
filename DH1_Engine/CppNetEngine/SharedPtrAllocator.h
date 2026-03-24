// ReSharper disable CppInconsistentNaming
#pragma once

template <typename T>
class SharedPtrAllocator final
{
public:

    using value_type = T;

	SharedPtrAllocator() = default;
	SharedPtrAllocator(const SharedPtrAllocator&) = default;
	SharedPtrAllocator(SharedPtrAllocator&&) = default;
	SharedPtrAllocator& operator=(const SharedPtrAllocator&) = default;
	SharedPtrAllocator& operator=(SharedPtrAllocator&&) = default;

	~SharedPtrAllocator() = default;

	template <typename U>
	// ReSharper disable once CppNonExplicitConvertingConstructor
	SharedPtrAllocator(const SharedPtrAllocator<U>&) {}

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
        MemoryAllocator::GetInstance().Free(ptr, sizeof(T) * size);
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

namespace cpp_net_engine
{
	template <typename T, typename... Args>
	SharedPtr<T> MakeShared(Args&&... args)
	{
		return SharedPtrUtils::Alloc<T>(std::forward<Args>(args)...);
	}
}
