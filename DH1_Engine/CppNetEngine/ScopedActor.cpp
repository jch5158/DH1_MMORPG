#include "pch.h"
#include "ScopedActor.h"

void ScopedActor::Dispatch(class IocpEvent& iocpEvent, const uint32)
{
}

bool ScopedActor::TryAcquire()
{
	bool bSuccess = false;
	const int32 retryLimit = GetRetryLimit();

	for (int32 retry = 0; retry < retryLimit; ++retry)
	{
		if (tryAcquireAll())
		{
			bSuccess = true;
			break;
		}

		Release();

		if (retry < 10)
		{
			_mm_pause();
		}
		else if (retry < 50)
		{
			std::this_thread::yield();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	if (!bSuccess)
	{
		Release();
	}

	return bSuccess;
}

void ScopedActor::Release()
{
	for (int32 i = mAcquireIndex; i >= 0; --i)
	{
		mActors[i]->Release();
	}

	mAcquireIndex = -1;
}

void ScopedActor::Activate()
{
	mMailbox.SetOwner(shared_from_this());
}

void ScopedActor::Register()
{
	if (TryAcquire())
	{
		mMailbox.Register();

		Release();
	}
}

void ScopedActor::Flush()
{
	if (TryAcquire())
	{
		mMailbox.Flush();

		Release();
	}
}

IocpEvent& ScopedActor::GetIocpEvent()
{
	return mMailbox.GetActorMessageEvent();
}

int32 ScopedActor::GetMessageCount()
{
	return mMailbox.GetMessageCount();
}

void ScopedActor::Post(MessageRef pMessage)
{
	mMailbox.Post(std::move(pMessage));
}

void ScopedActor::SetRetryLimit(const int32 retryLimit)
{
	mRetryLimit = retryLimit;
}

int32 ScopedActor::GetRetryLimit() const
{
	return mRetryLimit;
}

void ScopedActor::SetSpinLimit(const int32 spinCount)
{
	mSpinLimit = spinCount;
}

int32 ScopedActor::GetSpinLimit() const
{
	return mSpinLimit;
}

bool ScopedActor::tryAcquireAll()
{
	bool bAllAcquired = true; // 최종 결과를 담을 변수
	const int32 spinCount = GetSpinLimit();
	const int32 actorSize = static_cast<int32>(mActors.size());

	for (int32 i = 0; i < actorSize; ++i)
	{
		bool bAcquired = false;
		const int32 spinLimit = (i == 0) ? spinCount : 1;

		for (int32 curSpin = 0; curSpin < spinLimit; ++curSpin)
		{
			if (mActors[i]->TryAcquire())
			{
				mAcquireIndex = i;
				bAcquired = true;
				break;
			}

			_mm_pause(); // 하이퍼스레딩 최적화
		}

		if (!bAcquired)
		{
			bAllAcquired = false;
			break;
		}
	}

	return bAllAcquired;
}
