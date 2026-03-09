// ReSharper disable CppClangTidyClangDiagnosticPadded
#pragma once

#include "ObjectPool.h"

template <uint32 ALLOC_SIZE, uint32 ALIGN_SIZE = 16, int32 CHUNK_SIZE = 500>
class MemoryPool final
{
private:

	class Chunk final
	{
	public:

		static constexpr uint64 CHECKSUM_CODE = 0xDEADBEEFBEFFDEAD;

		struct ChunkData
		{
			ChunkData() = default;

			alignas(ALIGN_SIZE) byte data[ALLOC_SIZE];
			uint64 mChecksum = 0;
			Chunk* mpChunk = nullptr;
		};

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;
		Chunk(Chunk&&) = delete;
		Chunk& operator=(Chunk&&) = delete;

		explicit Chunk()
			: mAllocCount(0)
			, mFreeCount(0)
		{
			for (int32 i = 0; i < CHUNK_SIZE; ++i)
			{
				mChunkDataArray[i].mChecksum = CHECKSUM_CODE;
				mChunkDataArray[i].mpChunk = this;
			}
		}

		~Chunk() = default;

		void* GetData()
		{
			if (mAllocCount >= CHUNK_SIZE)
			{
				return nullptr;
			}

			return mChunkDataArray[mAllocCount++].data;
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
			return chunkData.mChecksum == CHECKSUM_CODE;
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
		:mObjectPool(0)
	{
		static_assert(ALLOC_SIZE > 0, "ALLOC_SIZE must be non-negative");
		static_assert(CHUNK_SIZE > 0, "CHUNK_SIZE must be non-negative");
		static_assert(std::is_standard_layout_v<ChunkBlock>, "ChunkBlock must be standard layout.");
		static_assert(offsetof(ChunkBlock, data) == 0, "data MUST be the first member for safe casting!");
	}

	~MemoryPool() = default;

	[[nodiscard]]
	void* Alloc()
	{
		if (spTlsChunk == nullptr)
		{
			spTlsChunk = mObjectPool.Alloc();
		}

		void* pRawMemory = spTlsChunk->GetData();
		if (pRawMemory == nullptr)
		{
			spTlsChunk = mObjectPool.Alloc();
			pRawMemory = spTlsChunk->GetData();
		}

		return pRawMemory;
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
			NET_ENGINE_LOG_ERROR("MemoryPool::Free - Invalid object detected. Possible memory corruption. checksum : {}", pChunkBlock->mChecksum);
			return;
		}

		Chunk* pChunk = pChunkBlock->mpChunk;
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

	void AllFree()
	{
		if (spTlsChunk != nullptr)
		{
			spTlsChunk->ChunkReset();
			mObjectPool.Free(spTlsChunk);
			spTlsChunk = nullptr;
		}
	}

private:

	inline static thread_local Chunk* spTlsChunk = nullptr;

	ObjectPool<Chunk> mObjectPool;
};
