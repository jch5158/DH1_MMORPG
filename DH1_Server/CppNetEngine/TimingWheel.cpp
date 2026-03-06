#include "pch.h"
#include "TimingWheel.h"

#include <algorithm>

TimerHandle::TimerHandle()
	:mpCancelFlag(cpp_net_engine::MakeShared<std::atomic<bool>>(false))
{
}

void TimerHandle::Cancel() const
{
	if (mpCancelFlag != nullptr)
	{
		mpCancelFlag->store(true);
	}
}

SharedPtr<std::atomic<bool>> TimerHandle::GetCancelFlag() const
{
	return mpCancelFlag;
}

TimingWheel::TimingNode::TimingNode(const uint64 expireTick, SharedPtr<std::atomic<bool>> pCancelFlag, std::function<void()> pCallback)
	: mExpireTick(expireTick)
	, mpCancelFlag(std::move(pCancelFlag))
	, mpCallback(std::move(pCallback))
{
}

uint64 TimingWheel::TimingNode::GetExpiredTick() const
{
	return mExpireTick;
}

void TimingWheel::TimingNode::Execute() const
{
	if (mpCancelFlag == nullptr || mpCancelFlag->load() == true)
	{
		return;
	}

	if (mpCallback != nullptr)
	{
		mpCallback();
	}
}

TimingWheel::TimingWheel(const uint64 tickInterval)
	: mbTicking(false)
	, mTickIntervalMs(std::max(tickInterval, DEFAULT_TICK_INTERVAL))
	, mLock()
	, mCurrentTick(0)
	, mLastTickTime(std::chrono::steady_clock::now())
{
	mWheel0.resize(LEVEL0_SIZE);
	mWheel1.resize(LEVEL1_SIZE);
	mWheel2.resize(LEVEL2_SIZE);
}

TimerHandle TimingWheel::AddTiming(std::function<void()> pCallback, const uint64 delayMs)
{
	TimerHandle handle;

	if (delayMs == 0)
	{
		pCallback();
		return handle;
	}


	{
		const uint64 safeDelayMs = std::min(delayMs, GetMaxDelayMs());
		const uint64 delayTicks = safeDelayMs / mTickIntervalMs;

		UniqueLock lock(mLock);
		TimingNode node(mCurrentTick + delayTicks, handle.GetCancelFlag(), std::move(pCallback));
		addNode(std::move(node));
	}

	return handle;
}

void TimingWheel::Tick()
{
	if (mbTicking.exchange(true) == true)
	{
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	const auto elapsedMs = static_cast<uint64>(std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastTickTime).count());
	if (std::cmp_less(elapsedMs, mTickIntervalMs))
	{
		mbTicking.store(false);
		return;
	}

	const uint64 tickCount = elapsedMs / mTickIntervalMs;
	mLastTickTime += std::chrono::milliseconds(tickCount * mTickIntervalMs);

	List<TimingNode> executeList;

	for (uint32 i = 0; i < tickCount; ++i)
	{
		UniqueLock lock(mLock);
		processTick(executeList);
		++mCurrentTick;
	}

	mbTicking.store(false);

	for (auto& node : executeList)
	{
		node.Execute();
	}
}

uint64 TimingWheel::GetMaxDelayMs() const
{
	return mTickIntervalMs * LEVEL0_SIZE * LEVEL1_SIZE * LEVEL2_SIZE;
}

void TimingWheel::processTick(List<TimingNode>& outExecuteList)
{
	const uint32 depth0 = mCurrentTick & LEVEL0_MASK;

	if (depth0 == 0 && mCurrentTick > 0)
	{
		const uint32 depth1 = (mCurrentTick >> LEVEL0_BITS) & LEVEL1_MASK;
		cascade(mWheel1[depth1]);

		if (depth1 == 0)
		{
			const uint32 depth2 = (mCurrentTick >> (LEVEL0_BITS + LEVEL1_BITS)) & LEVEL2_MASK;
			cascade(mWheel2[depth2]);
		}
	}

	outExecuteList.splice(outExecuteList.end(), mWheel0[depth0]);
}

void TimingWheel::addNode(TimingNode&& node)
{
	const uint64 delay = node.GetExpiredTick() - mCurrentTick;

	if (delay < LEVEL0_SIZE)
	{
		mWheel0[node.GetExpiredTick() & LEVEL0_MASK].push_back(std::move(node));
	}
	else if (delay < (LEVEL0_SIZE << LEVEL1_BITS))
	{
		mWheel1[(node.GetExpiredTick() >> LEVEL0_BITS) & LEVEL1_MASK].push_back(std::move(node));
	}
	else
	{
		mWheel2[(node.GetExpiredTick() >> (LEVEL0_BITS + LEVEL1_BITS)) & LEVEL2_MASK].push_back(std::move(node));
	}
}

void TimingWheel::cascade(List<TimingNode>& bucket)
{
	List<TimingNode> tempBucket;

	tempBucket.splice(tempBucket.end(), bucket);

	for (auto& node : tempBucket)
	{
		addNode(std::move(node));
	}
}