#pragma once

#include <utility>

#include "ObjectAllocator.h"
#include "SendBufferAllocator.h"

namespace cpp_net_engine
{
	template <typename T, typename... Args>
	T* NewObject(Args&&... args)
	{
		return ObjectAllocator<T>::GetInstance().Alloc(std::forward<Args>(args)...);
	}

	template <typename T>
	void DeleteObject(T* pData)
	{
		ObjectAllocator<T>::GetInstance().Free(pData);
	}

	inline void* RawAlloc(const int64 size)
	{
		return MemoryAllocator::GetInstance().Alloc(size);
	}

	inline NetSendBufferRef MakeSendBuffer(const int32 size)
	{
		return SendBufferAllocator::Alloc(size);
	}

	template <typename T, typename... Args>
	UniquePtr<T> MakeUnique(Args&&... args)
	{
		return UniquePtrUtils<T>::Alloc(std::forward<Args>(args)...);
	}

	template <typename T>
	UniquePtr<T[]> MakeUniqueArray(const int32 count)
	{
		return UniquePtrUtils<T[]>::Alloc(count);
	}

	template <typename T, typename... Args>
	std::shared_ptr<T> MakeShared(Args&&... args)
	{
		return SharedPtrUtils::Alloc<T>(std::forward<Args>(args)...);
	}
}
