#include "pch.h"
#include "Sender.h"
#include "Session.h"
#include "SocketUtils.h"

Sender::Sender()
	: mpOwner(nullptr)
	, mbSendRegistered(false)
	, mSendQueue()
{
}

void Sender::SetOwner(const SessionRef& pOwner)
{
	mpOwner = pOwner;
	mSendEvent.SetOwner(pOwner);
}

void Sender::Send(const NetSendBufferRef& pSendBuffer)
{
	if (mpOwner == nullptr || mpOwner->IsDisconnected())
	{
		return;
	}

	if (mSendQueue.TryEnqueue(pSendBuffer) == false)
	{
		NET_ENGINE_LOG_FATAL("Sender::Send - TryEnqueue is Failed, mSendQueue.Count() : {}", mSendQueue.Count());
		mpOwner->Disconnect(eDisconnectReason::ServiceError);
		return;
	}

	Register();
}

void Sender::Process(const uint32 numOfBytes)
{
	ClearEvent();

	if (mpOwner == nullptr || mpOwner->IsDisconnected())
	{
		return;
	}

	if (numOfBytes == 0)
	{
		mpOwner->Disconnect(eDisconnectReason::Closed);
		return;
	}

	mpOwner->OnSend(static_cast<int32>(numOfBytes));

	mbSendRegistered.store(false);

	if (!mSendQueue.IsEmpty())
	{
		Register();
	}
}

void Sender::Register()
{
	while (true)
	{
		if (mpOwner == nullptr || mpOwner->IsDisconnected())
		{
			return;
		}

		if (mbSendRegistered.exchange(true) == true)
		{
			return;
		}

		WSABUF wsabufs[MAX_SEND_WSABUF_SIZE]{};
		int32 sendCount;
		for (sendCount = 0; sendCount < MAX_SEND_WSABUF_SIZE; ++sendCount)
		{
			NetSendBufferRef pSendBuffer;
			if (mSendQueue.TryDequeue(pSendBuffer) == false)
			{
				break;
			}

			mSendEvent.GetSendPendingBuffer().emplace_back(pSendBuffer);
			wsabufs[sendCount].buf = reinterpret_cast<char*>(pSendBuffer->GetReadPtr());
			wsabufs[sendCount].len = pSendBuffer->GetUseSize();
		}

		if (sendCount == 0)
		{
			mbSendRegistered.store(false); // 플래그를 내림

			if (mSendQueue.IsEmpty() == false)
			{
				continue;
			}

			return;
		}

		mSendEvent.ClearOverlapped();
		mSendEvent.SetOwner(mpOwner);
		if (SocketUtils::WsaSend(mpOwner->GetSocket(), wsabufs, sendCount, &mSendEvent) == false)
		{
			const int32 errorCode = WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				mpOwner->OnError(errorCode);
				mpOwner->Disconnect(eDisconnectReason::SocketError);
				ClearEvent();
				Clear();
			}
		}

		return;
	}
}

void Sender::Clear()
{
	mpOwner.reset();
	mbSendRegistered.store(false);
	mSendQueue.Clear();
}

void Sender::ClearEvent()
{
	mSendEvent.GetSendPendingBuffer().clear();
	mSendEvent.ResetOwner();
}
