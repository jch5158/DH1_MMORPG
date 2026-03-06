#pragma once
#include "NetEngineLogger.h"

template <typename T>
class ObjectPool final
{
private:
	static constexpr uint64 CHECKSUM_CODE = 0xDEADBEEFBEFFDEAD;

	struct Node
	{
		T data{};
		uint64 checksum = 0;
		Node* pNextNode = nullptr;
	};

	struct Node16
	{
		Node* pNode = nullptr;
		int64 count = 0;
	};

public:
	
	ObjectPool(const ObjectPool&) = delete;
	ObjectPool& operator=(const ObjectPool&) = delete;
	ObjectPool(ObjectPool&&) = delete;
	ObjectPool& operator=(ObjectPool&&) = delete;

	explicit ObjectPool(const bool bPlacementNew, const int32 poolingCount)
		: mbPlacementNew(bPlacementNew)
		, mTopNode16{}
		, mPoolingCount(poolingCount)
	{
		static_assert(std::is_class_v<T>, "T is not class type.");

		for (int32 i = 0; i < mPoolingCount.load(); ++i)
		{
			Node* pNode = allocNode(!mbPlacementNew);

			pNode->checksum = CHECKSUM_CODE;
			pNode->pNextNode = mTopNode16.pNode;
			mTopNode16.pNode = pNode;
		}
	}

	~ObjectPool()
	{
		Node* pNode = mTopNode16.pNode;

		while (pNode != nullptr)
		{
			Node* pNextNode = pNode->pNextNode;

			if (!mbPlacementNew)
			{
				if constexpr (std::is_class_v<T>)
				{
					pNode->data.~T();
				}
			}

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
			pNode->checksum = CHECKSUM_CODE;
			pNode->pNextNode = nullptr;

			return &pNode->data;
		}

		Node16 expected{};
		Node16 desired{};

		std::atomic_ref<Node16> topNode16(mTopNode16);

		do
		{
			expected.count = mTopNode16.count;
			std::atomic_thread_fence(std::memory_order_seq_cst);
			expected.pNode = mTopNode16.pNode;
			
			desired.count = expected.count + 1;
			desired.pNode = expected.pNode->pNextNode;

		} while (topNode16.compare_exchange_weak(expected, desired) == false);

		if (mbPlacementNew)
		{
			new(&expected.pNode->data)T(std::forward<Args>(args)...);
		}

		return &expected.pNode->data;
	}

	void Free(T* pData)
	{
		if (pData == nullptr)
		{
			NET_ASSERT(false, " ObjectPool::Free - pData is nullptr.");
			return;
		}
		
		Node* pExpected;
		Node* pDesired = reinterpret_cast<Node*>(pData);
		if (pDesired->checksum != CHECKSUM_CODE)
		{
			NET_ASSERT(false, "ObjectPool::Free - Invalid object detected. Possible memory corruption.");
			return;
		}

		if (mbPlacementNew)
		{
			if constexpr (std::is_class_v<T>)
			{
				pDesired->data.~T();
			}
		}

		std::atomic_ref<Node*> topNodePtr(mTopNode16.pNode);

		do
		{
			pExpected = mTopNode16.pNode;
			pDesired->pNextNode = pExpected;

		} while (topNodePtr.compare_exchange_weak(pExpected, pDesired) == false);

		mPoolingCount.fetch_add(1);
	}

	[[nodiscard]]
	int32 PoolingCount() const
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
			new(&pNode->data)T(std::forward<Args>(args)...);
		}

		return pNode;
	}

	const bool mbPlacementNew;
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
			T data{};
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

	TlsObjectPool(const TlsObjectPool&) = delete;
	TlsObjectPool& operator=(const TlsObjectPool&) = delete;
	TlsObjectPool(TlsObjectPool&&) = delete;
	TlsObjectPool& operator=(TlsObjectPool&&) = delete;

	explicit TlsObjectPool()
		: mAllocCount(0)
		, mObjectPool(false, 0)
	{
		static_assert(std::is_class_v<T>, "T is not class type.");
		static_assert(CHUNK_SIZE > 0, "CHUNK_SIZE must be non-negative");
	}

	~TlsObjectPool() = default;

	template <typename... Args>
	T* Alloc(Args&&... args)
	{
		mAllocCount.fetch_add(1);

		if (spTlsChunk == nullptr)
		{
			spTlsChunk = mObjectPool.Alloc(std::forward<Args>(args)...);
		}

		T* pData = spTlsChunk->GetData();
		if (pData != nullptr && spTlsChunk->IsDataEmpty())
		{
			spTlsChunk = mObjectPool.Alloc();
		}

		return pData;
	}

	void Free(T* pData)
	{
		if (pData == nullptr)
		{
			NET_ASSERT(false, "TlsObjectPool::Free - pData is nullptr.");
			return;
		}

		ChunkBlock* pChunkBlock = reinterpret_cast<ChunkBlock*>(pData);
		if (!Chunk::IsValidChecksum(*pChunkBlock))
		{
			NET_ENGINE_LOG_ERROR("TlsObjectPool::Free - Invalid object detected. Possible memory corruption. checksum : {}", pChunkBlock->checksum);
			return;
		}

		Chunk* pChunk = pChunkBlock->pChunk;
		if (pChunk->FreeWithIsAllFreed())
		{
			pChunk->ChunkReset();
			mObjectPool.Free(pChunk);
		}

		mAllocCount.fetch_sub(1);
	}

	void AllFree()
	{
		spTlsChunk->ChunkReset();
		mObjectPool.Free(spTlsChunk);
		spTlsChunk = nullptr;
	}

	[[nodiscard]]
	int32 AllocCount() const
	{
		return mAllocCount.load();
	}

private:

	inline static thread_local Chunk* spTlsChunk = nullptr;
	std::atomic<int32> mAllocCount;
	ObjectPool<Chunk> mObjectPool;
};