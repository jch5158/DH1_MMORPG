// ReSharper disable CppClangTidyClangDiagnosticPadded
#pragma once

#include "ObjectPool.h"

template <uint32 ALLOC_SIZE, int32 CHUNK_SIZE = 500>
class MemoryPool final
{
private:

	class Chunk final
	{
	public:

		static constexpr uint64 CHECKSUM_CODE = 0xDEADBEEFBEFFDEAD;

		struct ChunkData
		{
			alignas(16) byte data[ALLOC_SIZE]{};
			uint64 checksum = 0;
			Chunk* pChunk = nullptr;
		};

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;
		Chunk(Chunk&&) = delete;
		Chunk& operator=(Chunk&&) = delete;

		explicit Chunk()
			:mAllocCount(0)
			, mFreeCount(0)
			, mChunkDataArray{}
		{
			for (int32 i = 0; i < CHUNK_SIZE; ++i)
			{
				mChunkDataArray[i].checksum = CHECKSUM_CODE;
				mChunkDataArray[i].pChunk = this;
			}
		}

		~Chunk() = default;

		void* GetData()
		{
			if (mAllocCount >= CHUNK_SIZE)
			{
				return nullptr;
			}

			return &mChunkDataArray[mAllocCount++].data;
		}

		bool IsDataEmpty() const
		{
			return mAllocCount == CHUNK_SIZE;
		}

		bool FreeWithIsAllFreed()
		{
			if (mFreeCount.fetch_add(1) == CHUNK_SIZE - 1)
			{
				return true;
			}

			return false;
		}

		void ChunkReset()
		{
			mAllocCount = 0;
			mFreeCount.store(0);
		}

		static bool IsValidChecksum(const ChunkData& chunkData)
		{
			return chunkData.checksum == CHECKSUM_CODE;
		}

	public:

		int32 mAllocCount;
		std::atomic<int32> mFreeCount;
		ChunkData mChunkDataArray[CHUNK_SIZE];
	};

	using ChunkBlock = Chunk::ChunkData;

public:

	MemoryPool(const MemoryPool&) = delete;
	MemoryPool& operator=(const MemoryPool&) = delete;
	MemoryPool(MemoryPool&&) = delete;
	MemoryPool& operator=(MemoryPool&&) = delete;

	explicit MemoryPool()
		:mObjectPool(false, 0)
	{
		static_assert(ALLOC_SIZE > 0, "ALLOC_SIZE must be non-negative");
		static_assert(CHUNK_SIZE > 0, "CHUNK_SIZE must be non-negative");
	}

	~MemoryPool() = default;

	[[nodiscard]]
	void* Alloc()
	{
		if (spTlsChunk == nullptr)
		{
			spTlsChunk = mObjectPool.Alloc();
		}

		void* pData = spTlsChunk->GetData();
		if (pData != nullptr && spTlsChunk->IsDataEmpty())
		{
			spTlsChunk = mObjectPool.Alloc();
		}

		return pData;
	}

	void Free(void* pData)
	{
		if (pData == nullptr)
		{
			NET_ASSERT(false, "MemoryPool::Free - pData is nullptr");
			return;
		}

		ChunkBlock* pChunkBlock = static_cast<ChunkBlock*>(pData);
		if (!Chunk::IsValidChecksum(*pChunkBlock))
		{
			NET_ASSERT(false, "MemoryPool::Free - Invalid checksum detected. Possible memory corruption.");
			return;
		}

		Chunk* pChunk = pChunkBlock->pChunk;
		if (pChunk->FreeWithIsAllFreed())
		{
			pChunk->ChunkReset();
			mObjectPool.Free(pChunk);
		}
	}

	[[nodiscard]]
	int32 AllocCount() const
	{
		return mObjectPool.AllocCount();
	}

	[[nodiscard]]
	int32 PoolingCount() const
	{
		return mObjectPool.PoolingCount();
	}

private:

	inline static thread_local Chunk* spTlsChunk = nullptr;

	ObjectPool<Chunk> mObjectPool;
};
