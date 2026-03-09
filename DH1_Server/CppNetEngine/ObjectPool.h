#pragma once
#include "NetEngineLogger.h"

template <typename T>
class ObjectPool final
{
private:
	static constexpr uint64 CHECKSUM_CODE = 0xDEADBEEFBEFFDEAD;

	struct Node
	{
		Node() = default;

		alignas(T) byte mData[sizeof(T)];
		uint64 mChecksum = 0;
		Node* mpNextNode = nullptr;
	};

	struct Node16
	{
		Node16() = default;

		Node* mpNode;
		int64 mCount = 0;
	};

public:
	
	ObjectPool(const ObjectPool&) = delete;
	ObjectPool& operator=(const ObjectPool&) = delete;
	ObjectPool(ObjectPool&&) = delete;
	ObjectPool& operator=(ObjectPool&&) = delete;

	explicit ObjectPool(const int32 poolingCount)
		: mTopNode16()
		, mPoolingCount(poolingCount)
	{
		static_assert(std::is_class_v<T>, "T is not class type.");
		static_assert(std::is_standard_layout_v<Node>, "Node must be standard layout.");
		static_assert(offsetof(Node, mData) == 0, "mData MUST be the first member for safe reinterpret_cast!");

		for (int32 i = 0; i < mPoolingCount.load(); ++i)
		{
			Node* pNode = allocNode(false);

			pNode->mChecksum = CHECKSUM_CODE;
			pNode->mpNextNode = mTopNode16.mpNode;
			mTopNode16.mpNode = pNode;
		}
	}

	~ObjectPool()
	{
		Node* pNode = mTopNode16.mpNode;

		while (pNode != nullptr)
		{
			Node* pNextNode = pNode->mpNextNode;

			mi_free(pNode);

			pNode = pNextNode;

			mPoolingCount.fetch_sub(1);
		}

		NET_ASSERT(mPoolingCount.load() == 0, "ObjectPool::~ObjectPool - Memory leak detected.");
	}


	template <typename... Args>
	T* Alloc(Args&&... args)
	{
		if (mPoolingCount.fetch_sub(1) <= 0)
		{
			mPoolingCount.fetch_add(1);

			Node* pNode = allocNode(true, std::forward<Args>(args)...);
			pNode->mChecksum = CHECKSUM_CODE;
			pNode->mpNextNode = nullptr;

			return reinterpret_cast<T*>(&pNode->mData);
		}

		Node16 expected;
		Node16 desired;

		std::atomic_ref<Node16> topNode16(mTopNode16);

		do
		{
			expected.mCount = mTopNode16.mCount;
			std::atomic_thread_fence(std::memory_order_seq_cst);
			expected.mpNode = mTopNode16.mpNode;
			
			desired.mCount = expected.mCount + 1;
			desired.mpNode = expected.mpNode->mpNextNode;

		} while (topNode16.compare_exchange_weak(expected, desired) == false);

		new(&expected.mpNode->mData) T(std::forward<Args>(args)...);

		return reinterpret_cast<T*>(&expected.mpNode->mData);
	}

	void Free(T* pData)
	{
		if (pData == nullptr)
		{
			NET_ENGINE_LOG_ERROR("ObjectPool::Free - pData is nullptr");
			return;
		}
		
		Node* pExpected;
		Node* pDesired = reinterpret_cast<Node*>(pData);
		if (pDesired->mChecksum != CHECKSUM_CODE)
		{
			NET_ENGINE_LOG_FATAL("ObjectPool::Free - Invalid object detected. Possible memory corruption.");
			return;
		}
		
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			pData->~T();
		}

		std::atomic_ref<Node*> topNodePtr(mTopNode16.mpNode);

		do
		{
			pExpected = mTopNode16.mpNode;
			pDesired->mpNextNode = pExpected;

		} while (topNodePtr.compare_exchange_weak(pExpected, pDesired) == false);

		mPoolingCount.fetch_add(1);
	}

	[[nodiscard]] int32 PoolingCount() const
	{
		return mPoolingCount.load();
	}

private:

	template <typename... Args>
	Node* allocNode(const bool bPlacementNew, Args&&... args)
	{
		Node* pNode = static_cast<Node*>(mi_malloc(sizeof(Node)));
		if (pNode == nullptr)
		{
			NET_ASSERT(false, "ObjectPool::allocNode - Memory allocation failed.");
			return nullptr;
		}
		
		if (bPlacementNew)
		{
			new(&pNode->mData)T(std::forward<Args>(args)...);
		}

		return pNode;
	}

	alignas(std::hardware_constructive_interference_size) Node16 mTopNode16;
	alignas(std::hardware_constructive_interference_size) std::atomic<int32> mPoolingCount;
};


template <typename T, int32 CHUNK_SIZE = 500>
class TlsObjectPool final
{
private:

	class Chunk final
	{
	public:

		static constexpr uint64 CHECKSUM_CODE = 0xDEADBEEFBEFFDEAD;

		struct ChunkData
		{
			ChunkData() = default;

			alignas(T) byte mData[sizeof(T)];
			uint64 mChecksum = 0;
			Chunk* mpChunk = nullptr;
		};

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;
		Chunk(Chunk&&) = delete;
		Chunk& operator=(Chunk&&) = delete;

		explicit Chunk()
			:mAllocCount(0)
			, mFreeCount(0)
			, mChunkDataArray()
		{
			for (int32 i = 0; i < CHUNK_SIZE; ++i)
			{
				mChunkDataArray[i].mChecksum = CHECKSUM_CODE;
				mChunkDataArray[i].mpChunk = this;
			}
		}

		~Chunk()
		{
			ChunkReset();
		}

		T* GetData()
		{
			if (mAllocCount >= CHUNK_SIZE)
			{
				return nullptr;
			}

			return reinterpret_cast<T*>(&mChunkDataArray[mAllocCount++].mData);
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

	TlsObjectPool(const TlsObjectPool&) = delete;
	TlsObjectPool& operator=(const TlsObjectPool&) = delete;
	TlsObjectPool(TlsObjectPool&&) = delete;
	TlsObjectPool& operator=(TlsObjectPool&&) = delete;

	explicit TlsObjectPool()
		: mAllocCount(0)
		, mObjectPool(0)
	{
		static_assert(std::is_class_v<T>, "T is not class type.");
		static_assert(CHUNK_SIZE > 0, "CHUNK_SIZE must be non-negative");
		static_assert(std::is_standard_layout_v<ChunkBlock>, "ChunkBlock must be standard layout.");
		static_assert(offsetof(ChunkBlock, mData) == 0, "mData MUST be the first member for safe reinterpret_cast!");
	}

	~TlsObjectPool() = default;

	template <typename... Args>
	T* Alloc(Args&&... args)
	{
		mAllocCount.fetch_add(1);

		if (spTlsChunk == nullptr)
		{
			spTlsChunk = mObjectPool.Alloc();
		}

		T* pRawMemory = spTlsChunk->GetData();
		if (pRawMemory == nullptr)
		{
			spTlsChunk = mObjectPool.Alloc();
			pRawMemory = spTlsChunk->GetData();
		}

		new(pRawMemory) T(std::forward<Args>(args)...);
		return pRawMemory;
	}

	void Free(T* pObject)
	{
		if (pObject == nullptr)
		{
			NET_ASSERT(false, "TlsObjectPool::Free - pData is nullptr.");
			return;
		}

		ChunkBlock* pChunkBlock = reinterpret_cast<ChunkBlock*>(pObject);
		if (!Chunk::IsValidChecksum(*pChunkBlock))
		{
			NET_ENGINE_LOG_ERROR("TlsObjectPool::Free - Invalid object detected. Possible memory corruption. checksum : {}", pChunkBlock->mChecksum);
			return;
		}

		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			pObject->~T();
		}

		Chunk* pChunk = pChunkBlock->mpChunk;
		if (pChunk->FreeWithIsAllFreed())
		{
			pChunk->ChunkReset();
			mObjectPool.Free(pChunk);
		}

		mAllocCount.fetch_sub(1);
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

	[[nodiscard]] int32 AllocCount() const
	{
		return mAllocCount.load();
	}

private:

	inline static thread_local Chunk* spTlsChunk = nullptr;

	std::atomic<int32> mAllocCount;
	ObjectPool<Chunk> mObjectPool;
};