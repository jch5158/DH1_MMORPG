#include "pch.h"
#include "SessionIdAllocator.h"
#include "RedisConnection.h"

SessionIdAllocator::SessionIdAllocator()
	: mpRedisConnection(nullptr)
	, mBlockLock()
	, mNextId(0)
	, mBlockEnd(0)
{
}

bool SessionIdAllocator::Initialize(RedisConnection* pRedisConnection)
{
	mpRedisConnection = pRedisConnection;
	return allocateBlock();
}

uint64 SessionIdAllocator::Allocate()
{
	UniqueLock lock(mBlockLock);

	if (mNextId > mBlockEnd)
	{
		if (!allocateBlock())
		{
			return 0;
		}
	}

	return mNextId++;
}

bool SessionIdAllocator::allocateBlock()
{
	if (mpRedisConnection == nullptr)
	{
		static std::atomic<uint64> sLocalCounter(0);
		mNextId = sLocalCounter.fetch_add(BLOCK_SIZE) + 1;
		mBlockEnd = mNextId + BLOCK_SIZE - 1;
		return true;
	}

	const int64 blockEnd = mpRedisConnection->IncrBy(SESSION_ID_REDIS_KEY, BLOCK_SIZE);
	if (blockEnd == 0)
	{
		NET_ENGINE_LOG_ERROR("[SessionIdAllocator] Failed to allocate block from Redis");
		return false;
	}

	mNextId = static_cast<uint64>(blockEnd - BLOCK_SIZE + 1);
	mBlockEnd = static_cast<uint64>(blockEnd);

	NET_ENGINE_LOG_INFO("[SessionIdAllocator] Block allocated: [{}, {}]", mNextId, mBlockEnd);
	return true;
}
