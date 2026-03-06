#pragma once

#include "ISingleton.h"
#include "ObjectPool.h"

template <typename T, int32 CHUNK_SIZE = 500>
class ObjectAllocator final : public ISingleton<ObjectAllocator<T, CHUNK_SIZE>>
{
public:

	friend class ISingleton<ObjectAllocator>;

	ObjectAllocator(const ObjectAllocator&) = delete;
	ObjectAllocator& operator=(const ObjectAllocator&) = delete;
	ObjectAllocator(ObjectAllocator&&) = delete;
	ObjectAllocator& operator=(ObjectAllocator&&) = delete;

private:

	ObjectAllocator()
		:mTlsObjectPool()
	{
		static_assert(std::is_class_v<T>, "T is not class type.");
		static_assert(CHUNK_SIZE > 0, "CHUNK_SIZE must be non-negative");
	}

public:

	~ObjectAllocator() = default;

	[[nodiscard]]
	int32 AllocCount() const
	{
		return mTlsObjectPool.AllocCount();
	}

	template <typename... Args>
	[[nodiscard]]
	T* Alloc(Args&&... args)
	{
		T* pData = mTlsObjectPool.Alloc(std::forward<Args>(args)...);

		return pData;
	}

	void Free(T* pData)
	{
		mTlsObjectPool.Free(pData);
	}

private:
	
	TlsObjectPool<T, CHUNK_SIZE> mTlsObjectPool;
};

