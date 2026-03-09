#pragma once

#include "pch.h"
#include "ObjectAllocator.h"

template <typename T>
class LockFreeStack final
{
public:

	struct Node
	{
		Node() = default;

		template <typename U>
		explicit Node(U&& data) requires (std::is_constructible_v<T, U>)
			: mData(std::forward<U>(data))
			, mpNextNode(nullptr)
		{
		}

		T mData;
		Node* mpNextNode = nullptr;
	};

private:

	struct Node16
	{
		Node16() = default;

		Node* mpNode = nullptr;
		int64 mCount = 0;
	};

public:

	LockFreeStack(const LockFreeStack&) = delete;
	LockFreeStack& operator=(const LockFreeStack&) = delete;
	LockFreeStack(LockFreeStack&&) = delete;
	LockFreeStack& operator=(LockFreeStack&&) = delete;

	explicit LockFreeStack(const int32 maxCount)
		: mMaxCount(maxCount)
		, mCount(0)
		, mTopAlineNode16()
	{
	}

	~LockFreeStack()
	{
		Node* pNode = mTopAlineNode16.mpNode;

		while (pNode != nullptr)
		{
			Node* pNextNode = pNode->mpNextNode;
			
			cpp_net_engine::DeleteObject(pNode);

			pNode = pNextNode;
		}
	}

	template <typename U>
	[[nodiscard]] bool TryPush(U&& data)
	{
		if (mMaxCount <= mCount.load())
		{
			return false;
		}

		Node* pExpected;
		Node* pDesired = cpp_net_engine::NewObject<Node>(std::forward<U>(data));

		std::atomic_ref<Node*> topNodePtr(mTopAlineNode16.mpNode);

		do
		{
			pExpected = mTopAlineNode16.mpNode;
			pDesired->mpNextNode = pExpected;

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
		
		Node16 expected;
		Node16 desired;

		std::atomic_ref<Node16> topAlign16Node(mTopAlineNode16);

		do
		{
			expected.mCount = mTopAlineNode16.mCount;
			std::atomic_thread_fence(std::memory_order_seq_cst);
			expected.mpNode = mTopAlineNode16.mpNode;

			desired.mCount = expected.mCount + 1;
			desired.mpNode = expected.mpNode->mpNextNode;

		} while (topAlign16Node.compare_exchange_weak(expected, desired) == false);

		outData = std::move(expected.mpNode->mData);
		cpp_net_engine::DeleteObject(expected.mpNode);

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
