#include "pch.h"
#include "SendBufferAllocator.h"

NetSendBufferRef SendBufferAllocator::Alloc(const int32 size)
{
	if (size < 0 || size > MAX_SIZE)
	{
		return nullptr;
	}

	const int32 allocSize = getAllocSize(size);

	NetSendBufferRef pData = SharedPtrUtils::Alloc<NetSendBuffer>(allocSize);

	return pData;
}

int32 SendBufferAllocator::getAllocSize(const int32 size)
{
	int32 allocSize;

	if (size <= THRESHOLD)
	{
		allocSize = (size + SMALL_STRIDE - 1) & ~(SMALL_STRIDE - 1);
	}
	else
	{
		allocSize = (size + LARGE_STRIDE - 1) & ~(LARGE_STRIDE - 1);
	}

	return allocSize;
}
