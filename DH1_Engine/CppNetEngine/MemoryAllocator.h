// ReSharper disable CppMemberFunctionMayBeConst
#pragma once

class MemoryAllocator final : public ISingleton<MemoryAllocator>
{
private:

	static constexpr int32 SMALL_STRIDE = 256;
	static constexpr int32 LARGE_STRIDE = 4096;
	static constexpr int32 THRESHOLD = 4096;
	static constexpr int32 MAX_SIZE = 65536;
	static constexpr int32 SMALL_POOL_COUNT = (THRESHOLD / SMALL_STRIDE) - 1;
	static constexpr int32 LARGE_POOL_COUNT = (MAX_SIZE / LARGE_STRIDE);
	static constexpr uint64 CHECKSUM_CODE = 0xDEAD0004BEEF0004;

public:

	friend class ISingleton<MemoryAllocator>;

	MemoryAllocator(const MemoryAllocator&) = delete;
	MemoryAllocator& operator=(const MemoryAllocator&) = delete;
	MemoryAllocator(MemoryAllocator&&) = delete;
	MemoryAllocator& operator=(MemoryAllocator&&) = delete;

private:

	explicit MemoryAllocator() = default;

public:

	~MemoryAllocator() = default;

	[[nodiscard]] void* Alloc(const int64 size)
	{
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
			void* pRaw = mi_malloc(sizeof(int64) + size + sizeof(uint64));
			if (pRaw == nullptr)
			{
				return nullptr;
			}

			std::memcpy(pRaw, &size, sizeof(int64));
			pData = static_cast<byte*>(pRaw) + sizeof(int64);
			setChecksum(pData, size);
		}

		return pData;
	}
	
	void Free(void* pData, const int64 size)
	{
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
			void* pRaw = static_cast<byte*>(pData) - sizeof(int64);
			int64 storedSize = 0;
			std::memcpy(&storedSize, pRaw, sizeof(int64));

			if (storedSize != size)
			{
				NET_ENGINE_LOG_ERROR("MemoryAllocator::Free - Size mismatch. stored: {}, requested: {}", storedSize, size);
				return;
			}

			if (!isValidChecksum(pData, storedSize))
			{
				NET_ENGINE_LOG_ERROR("MemoryAllocator::Free - Invalid checksum for large allocation.");
				return;
			}

			mi_free(pRaw);
		}
	}

private:

	static void setChecksum(void* pData, const int64 size)
	{
		byte* const pOffset = static_cast<byte*>(pData) + size;
		constexpr uint64 checksumValue = CHECKSUM_CODE;
		std::memcpy(pOffset, &checksumValue, sizeof(uint64));
	}

	[[nodiscard]] static bool isValidChecksum(const void* pData, const int64 size)
	{
		const byte* const pOffset = static_cast<const byte*>(pData) + size;
		uint64 extractedChecksum = 0;
		std::memcpy(&extractedChecksum, pOffset, sizeof(uint64));
		return extractedChecksum == CHECKSUM_CODE;
	}

	[[nodiscard]] static int32 getBucketIndex(const int64 size, const int32 stride)
	{
		const int64 index = (size - 1) / stride;
		return static_cast<int32>(index);
	}


	template <int32 STRIDE, typename SEQUENCE>
	struct TupleBuilder
	{
	};

	template <int32 STRIDE, int32... INDEX>
	struct TupleBuilder<STRIDE, std::index_sequence<INDEX...>>
	{
		using type = std::tuple<MemoryPool<(INDEX + 1)* STRIDE>...>;
	};

	template <typename T>
	class AllocActor
	{
	public:
		using FuncType = void* (*)(T&);

		template <int32 INDEX>
		static void* Do(T& buckets)
		{
			return std::get<INDEX>(buckets).Alloc();
		}
	};

	template <typename T>
	class FreeActor
	{
	public:
		using FuncType = void (*)(T&, void*);

		template <int32 INDEX>
		static void Do(T& buckets, void* pData) 
		{
			std::get<INDEX>(buckets).Free(pData);
		}
	};

	template <typename ACTION, int32... INDEX>
	static const auto& getTable(std::index_sequence<INDEX...>) 
	{
		constexpr int32 count = static_cast<int32>(sizeof...(INDEX));

		static const std::array<typename ACTION::FuncType, count> table
			= { &ACTION::template Do<INDEX>... };

		return table;
	}

	using SmallBucketsTuple = TupleBuilder<SMALL_STRIDE, std::make_index_sequence<SMALL_POOL_COUNT>>::type;
	using LargeBucketsTuple = TupleBuilder<LARGE_STRIDE, std::make_index_sequence<LARGE_POOL_COUNT>>::type;

	using SmallAllocActor = AllocActor<SmallBucketsTuple>;
	using SmallFreeActor = FreeActor<SmallBucketsTuple>;
	using LargeAllocActor = AllocActor<LargeBucketsTuple>;
	using LargeFreeActor = FreeActor<LargeBucketsTuple>;

	SmallBucketsTuple mSmallBuckets;
	LargeBucketsTuple mLargeBuckets;
};

namespace cpp_net_engine
{
	inline void* RawAlloc(const int64 size)
	{
		return MemoryAllocator::GetInstance().Alloc(size);
	}

	inline void RawFree(void* pData, const int64 size)
	{
		return MemoryAllocator::GetInstance().Free(pData, size);
	}

}