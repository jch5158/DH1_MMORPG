#pragma once

class RedisConnection;

class SessionIdAllocator
{
public:
	SessionIdAllocator(const SessionIdAllocator&) = delete;
	SessionIdAllocator& operator=(const SessionIdAllocator&) = delete;
	SessionIdAllocator(SessionIdAllocator&&) = delete;
	SessionIdAllocator& operator=(SessionIdAllocator&&) = delete;

	explicit SessionIdAllocator();
	~SessionIdAllocator() = default;

	bool Initialize(RedisConnection* pRedisConnection);
	uint64 Allocate();

private:
	bool allocateBlock();

	static constexpr int64 BLOCK_SIZE = 1000;
	static constexpr const char* SESSION_ID_REDIS_KEY = "session:id:counter";

	RedisConnection* mpRedisConnection;
	Mutex mBlockLock;
	uint64 mNextId;
	uint64 mBlockEnd;
};
