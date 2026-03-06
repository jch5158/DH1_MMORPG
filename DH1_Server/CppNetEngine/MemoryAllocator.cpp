#include "pch.h"
#include "MemoryAllocator.h"

void* MemoryAllocator::	Alloc(const int64 size)
{
	NET_ASSERT(size > 0, "MemoryAllocator::Free - is is zero or negative");

	if (size == 0)
	{
		return nullptr;
	}

	void* pData;

	if (size <= (THRESHOLD - SMALL_STRIDE))
	{
		const int32 index = getBucketIndex(size, SMALL_STRIDE);

		const auto& table = getTable<SmallAllocActor>(std::make_index_sequence<SMALL_POOL_COUNT>{});

		pData = table[index](mSmallBuckets);
	}
	else if (size <= MAX_SIZE)
	{
		const int32 index = getBucketIndex(size, LARGE_STRIDE);

		const auto& table = getTable<LargeAllocActor>(std::make_index_sequence<LARGE_POOL_COUNT>{});

		pData = table[index](mLargeBuckets);
	}
	else
	{
		pData = mi_malloc(size + sizeof(uint64));

		setChecksum(pData, size);
	}

	return pData;
}

void MemoryAllocator::Free(void* pData, const int64 size)
{
	NET_ASSERT(pData != nullptr, "MemoryAllocator::Free - pData is nullptr");

	if (pData == nullptr || size == 0)
	{
		return;
	}

	if (size <= (THRESHOLD - SMALL_STRIDE))
	{
		const int32 index = getBucketIndex(size, SMALL_STRIDE);

		const auto& table = getTable<SmallFreeActor>(std::make_index_sequence<SMALL_POOL_COUNT>{});

		table[index](mSmallBuckets, pData);
	}
	else if (size <= MAX_SIZE)
	{
		const int32 index = getBucketIndex(size, LARGE_STRIDE);

		const auto& table = getTable<LargeFreeActor>(std::make_index_sequence<LARGE_POOL_COUNT>{});

		table[index](mLargeBuckets, pData);
	}
	else
	{
		if (!isValidChecksum(pData, size))
		{
			NET_ASSERT(false, "MemoryAllocator::Free - Invalid checksum detected. Possible memory corruption.");

			return;
		}

		mi_free(pData);
	}
}

void MemoryAllocator::setChecksum(void* pData, const int64 size)
{
	auto* const checksumOffset = reinterpret_cast<uint64*>(static_cast<byte*>(pData) + size);

	*checksumOffset = CHECKSUM_CODE;
}

bool MemoryAllocator::isValidChecksum(void* pData, const int64 size)
{
	const auto* const checksumOffset = reinterpret_cast<uint64*>(static_cast<byte*>(pData) + size);

	return *checksumOffset == CHECKSUM_CODE;
}

int32 MemoryAllocator::getBucketIndex(const int64 size, const int32 stride)
{
	const int64 index = (size - 1) / stride;

	return static_cast<int32>(index);
}
