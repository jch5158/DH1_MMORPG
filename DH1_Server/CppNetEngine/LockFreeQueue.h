#pragma once

#include "ObjectAllocator.h"

template <typename T, int32 CHUNK_SIZE = 500>
class LockFreeQueue final
{
public:

	static constexpr int32 DEFAULT_MAX_COUNT = 65535;

	struct Node
	{
		T data{};
		Node* pNextNode = nullptr;
	};

private:

	struct Node16
	{
		Node* pNode = nullptr;
		int64 count = 0;
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
		pDummyNode->pNextNode = nullptr;
		mHead.pNode = pDummyNode;
		mTail.pNode = pDummyNode;
	}

	~LockFreeQueue()
	{
		Clear();
	}

	[[nodiscard]]
	bool TryEnqueue(const T& data)
	{
		if (mMaxCount <= mCount.load())
		{
			return false;
		}

		Node* pDesired = cpp_net_engine::NewObject<Node>();
		pDesired->data = data;
		pDesired->pNextNode = nullptr;

		while (true)
		{
			Node* pExpected = mTail.pNode->pNextNode;
			std::atomic_ref<Node*> atomicTailNextNodePtr(mTail.pNode->pNextNode);
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

	[[nodiscard]]
	bool TryDequeue(T& outData)
	{
		if (mCount.fetch_sub(1) <= 0)
		{
			mCount.fetch_add(1);
			return false;
		}

		Node16 expected{};
		Node16 desired{};

		std::atomic_ref<Node16> atomicHead(mHead);

		while (true)
		{
			if (mHead.pNode == mTail.pNode)
			{
				moveTail();
				continue;
			}

			expected.count = mHead.count;
			std::atomic_thread_fence(std::memory_order_seq_cst);
			expected.pNode = mHead.pNode;
			if (expected.pNode == nullptr)
			{
				continue;
			}

			desired.count = expected.count + 1;
			desired.pNode = expected.pNode->pNextNode;
			if (desired.pNode == nullptr)
			{
				continue;
			}

			outData = desired.pNode->data;

			if (atomicHead.compare_exchange_weak(expected, desired) == true)
			{
				break;
			}
		}

		cpp_net_engine::DeleteObject(expected.pNode);

		return true;
	}

	void Clear()
	{
		Node* pNode = mHead.pNode;

		while (pNode != nullptr)
		{
			Node* pNextNode = pNode->pNextNode;
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
		Node16 expected{};
		Node16 desired{};

		expected.count = mTail.count;
		std::atomic_thread_fence(std::memory_order_seq_cst);
		expected.pNode = mTail.pNode;

		desired.count = expected.count + 1;
		desired.pNode = expected.pNode->pNextNode;
		if (desired.pNode == nullptr)
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
