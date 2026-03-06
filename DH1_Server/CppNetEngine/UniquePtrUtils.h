#pragma once

#include "MemoryAllocator.h"

template <typename T>
class UniquePtrUtils final  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:

	// 삭제자(Deleter)는 T 타입에만 의존해야 합니다. (Args와 무관)
	class Deleter
	{
	public:
		void operator()(T* ptr) const
		{
			if constexpr (std::is_class_v<T>)
			{
				ptr->~T();
			}

			MemoryAllocator::GetInstance().Free(static_cast<void*>(ptr), sizeof(T));
		}
	};

	template <typename... Args>
	static std::unique_ptr<T, Deleter> Alloc(Args&&... args)
	{
		const uint64 memSize = sizeof(T);
		void* memPtr = MemoryAllocator::GetInstance().Alloc(memSize);

		T* returnPtr = new(memPtr) T(std::forward<Args>(args)...);

		return std::unique_ptr<T, Deleter>(returnPtr);
	}
};

// 2. [특수화] 배열용 (Array, 예: char[])
template <typename T>
class UniquePtrUtils<T[]> final
{
public:
	class ArrayDeleter
	{
	public:
		explicit ArrayDeleter(const uint64 size)
			: mSize(size)
		{
		}

		void operator()(T* ptr) const
		{
			if constexpr (std::is_class_v<T>)
			{
				const uint64 count = mSize / sizeof(T);
				for (uint64 i = 0; i < count; ++i)
				{
					ptr[count - 1 - i].~T();
				}
			}

			MemoryAllocator::GetInstance().Free(ptr, mSize);
		}

	private:
		uint64 mSize;
	};

	static std::unique_ptr<T[], ArrayDeleter> Alloc(const uint64 count)
	{
		const uint64 size = sizeof(T) * count;
		void* voidPtr = MemoryAllocator::GetInstance().Alloc(size);

		// [수정 2] void*를 T*로 변환해야 인덱싱 가능
		T* typePtr = static_cast<T*>(voidPtr);

		// 배열은 기본 생성자 호출이므로 Trivial 최적화 가능
		if constexpr (!std::is_trivially_default_constructible_v<T>)
		{
			for (uint64 i = 0; i < count; ++i)
			{
				new (&typePtr[i]) T();
			}
		}

		return std::unique_ptr<T[], ArrayDeleter>(typePtr, ArrayDeleter(size));
	}
};

// ==========================================
// 타입 헬퍼
// ==========================================
template <typename T>
struct GetUniquePtrType
{
	using Type = std::unique_ptr<T, typename UniquePtrUtils<T>::Deleter>;
};

template <typename T>
struct GetUniquePtrType<T[]>
{
	using Type = std::unique_ptr<T[], typename UniquePtrUtils<T[]>::ArrayDeleter>;
};

template <typename T>
using UniquePtr = GetUniquePtrType<T>::Type;

template <typename T>
using UniqueContPtr = GetUniquePtrType<const T>::Type;
