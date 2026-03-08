#pragma once

#include "ObjectAllocator.h"

template <typename T, int32 CHUNK_SIZE = 500>
class LockFreeQueue final
{
public:

	static constexpr int32 DEFAULT_MAX_COUNT = 65535;

	struct Node
	{
		explicit Node()
			: mData()
			, mpNextNode(nullptr)
		{
		}

		template <typename U>
		explicit Node(U&& data) requires (std::is_constructible_v<T, U>)
			: mData(std::forward<U>(data))
			, mpNextNode(nullptr)
		{
		}

		T mData;
		Node* mpNextNode;
	};

private:

	struct Node16
	{
		explicit Node16()
			:mpNode(nullptr)
			, mCount(0)
		{
		}

		Node* mpNode = nullptr;
		int64 mCount = 0;
	};

public:

	LockFreeQueue(const LockFreeQueue&) = delete;
	LockFreeQueue& operator=(const LockFreeQueue&) = delete;
	LockFreeQueue(LockFreeQueue&&) = delete;
	LockFreeQueue& operator=(LockFreeQueue&&) = delete;

	explicit LockFreeQueue(const int32 maxCount = DEFAULT_MAX_COUNT)
		: mMaxCount(maxCount)
		, mCount(0)
		, mHead{}
		, mTail{}
	{
		Node* pDummyNode = cpp_net_engine::NewObject<Node>();
		pDummyNode->mpNextNode = nullptr;
		mHead.mpNode = pDummyNode;
		mTail.mpNode = pDummyNode;
	}

	~LockFreeQueue()
	{
		Clear();
	}

	template <typename U>
	[[nodiscard]] bool TryEnqueue(U&& data)
	{
		if (mMaxCount <= mCount.load())
		{
			return false;
		}

		Node* pDesired = cpp_net_engine::NewObject<Node>(std::forward<U>(data));

		while (true)
		{
			Node* pExpected = mTail.mpNode->mpNextNode;
			std::atomic_ref<Node*> atomicTailNextNodePtr(mTail.mpNode->mpNextNode);
			if (pExpected == nullptr && (atomicTailNextNodePtr.compare_exchange_weak(pExpected, pDesired) == true))
			{
				moveTail();
				break;
			}
		
			moveTail();
		}

		mCount.fetch_add(1);
		
		return true;
	}

	[[nodiscard]] bool TryDequeue(T& outData)
	{
		if (mCount.fetch_sub(1) <= 0)
		{
			mCount.fetch_add(1);
			return false;
		}

		Node16 expected;
		Node16 desired;

		std::atomic_ref<Node16> atomicHead(mHead);

		while (true)
		{
			if (mHead.mpNode == mTail.mpNode)
			{
				moveTail();
				continue;
			}

			expected.mCount = mHead.mCount;
			std::atomic_thread_fence(std::memory_order_seq_cst);
			expected.mpNode = mHead.mpNode;
			if (expected.mpNode == nullptr)
			{
				continue;
			}

			desired.mCount = expected.mCount + 1;
			desired.mpNode = expected.mpNode->mpNextNode;
			if (desired.mpNode == nullptr)
			{
				continue;
			}

			outData = desired.mpNode->mData;

			if (atomicHead.compare_exchange_weak(expected, desired) == true)
			{
				break;
			}
		}

		cpp_net_engine::DeleteObject(expected.mpNode);

		return true;
	}

	void Clear()
	{
		Node* pNode = mHead.mpNode;

		while (pNode != nullptr)
		{
			Node* pNextNode = pNode->mpNextNode;
			cpp_net_engine::DeleteObject(pNode);
			pNode = pNextNode;
		}
	}

	bool IsEmpty() const
	{
		return mCount.load() == 0;
	}

	int32 Count() const
	{
		return mCount.load();
	}

	int32 MaxCount() const noexcept
	{
		return mMaxCount;
	}

private:

	void moveTail()
	{
		Node16 expected;
		Node16 desired;

		expected.mCount = mTail.mCount;
		std::atomic_thread_fence(std::memory_order_seq_cst);
		expected.mpNode = mTail.mpNode;

		desired.mCount = expected.mCount + 1;
		desired.mpNode = expected.mpNode->mpNextNode;
		if (desired.mpNode == nullptr)
		{
			return;
		}

		std::atomic_ref<Node16> atomicTail(mTail);
		atomicTail.compare_exchange_weak(expected, desired);
	}

	const int32 mMaxCount;

	alignas(std::hardware_destructive_interference_size) std::atomic<int32> mCount;

	alignas(std::hardware_destructive_interference_size) Node16 mHead;

	alignas(std::hardware_destructive_interference_size) Node16 mTail;
};
