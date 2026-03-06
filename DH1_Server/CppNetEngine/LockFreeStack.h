#pragma once

#include "pch.h"
#include "ObjectAllocator.h"

template <typename T, int32 CHUNK_SIZE = 500>
class LockFreeStack final
{
public:

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

	LockFreeStack(const LockFreeStack&) = delete;
	LockFreeStack& operator=(const LockFreeStack&) = delete;
	LockFreeStack(LockFreeStack&&) = delete;
	LockFreeStack& operator=(LockFreeStack&&) = delete;

	explicit LockFreeStack(const int32 maxCount)
		: mMaxCount(maxCount)
		, mCount(0)
		, mTopAlineNode16{}
	{
	}

	~LockFreeStack()
	{
		Node* pNode = mTopAlineNode16.pNode;

		while (pNode != nullptr)
		{
			Node* pNextNode = pNode->pNextNode;
			
			cpp_net_engine::DeleteObject(pNode);

			pNode = pNextNode;
		}
	}

	[[nodiscard]]
	bool TryPush(const T& data)
	{
		if (mMaxCount <= mCount.load())
		{
			return false;
		}

		Node* pExpected{};
		Node* pDesired = cpp_net_engine::NewObject<Node>();
		pDesired->data = data;

		std::atomic_ref<Node*> topNodePtr(mTopAlineNode16.pNode);

		do
		{
			pExpected = mTopAlineNode16.pNode;
			pDesired->pNextNode = pExpected;

		} while (topNodePtr.compare_exchange_weak(pExpected, pDesired) == false);

		mCount.fetch_add(1);

		return true;
	}

	[[nodiscard]]
	bool TryPop(T& outData)
	{
		if (mCount.fetch_sub(1) <= 0)
		{
			mCount.fetch_add(1);
			return false;
		}
		
		Node16 expected{};
		Node16 desired{};

		std::atomic_ref<Node16> topAlign16Node(mTopAlineNode16);

		do
		{
			expected.count = mTopAlineNode16.count;
			std::atomic_thread_fence(std::memory_order_seq_cst);
			expected.pNode = mTopAlineNode16.pNode;

			desired.count = expected.count + 1;
			desired.pNode = expected.pNode->pNextNode;

		} while (topAlign16Node.compare_exchange_weak(expected, desired) == false);

		outData = expected.pNode->data;

		cpp_net_engine::DeleteObject(expected.pNode);

		return true;
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

	const int32 mMaxCount;

	alignas(std::hardware_destructive_interference_size) std::atomic<int32> mCount;

	alignas(std::hardware_destructive_interference_size) Node16 mTopAlineNode16;
};
