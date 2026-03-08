#include "pch.h"
#include "Sender.h"
#include "Session.h"
#include "SocketUtils.h"

Sender::Sender()
	: mbSendRegistered(false)
	, mSendQueue()
{
}

void Sender::SetOwner(const SessionRef& pOwner)
{
	if (pOwner == nullptr)
	{
		return;
	}

	mSendEvent.SetOwner(pOwner);
}

void Sender::Send(const NetSendBufferRef& pSendBuffer)
{
	const SessionRef pOwner = static_pointer_cast<Session>(mSendEvent.GetOwner());
	if (pOwner == nullptr || pOwner->IsDisconnected())
	{
		return;
	}

	if (mSendQueue.TryEnqueue(pSendBuffer) == false)
	{
		NET_ENGINE_LOG_FATAL("Sender::Send - TryEnqueue is Failed, mSendQueue.Count() : {}", mSendQueue.Count());
		pOwner->Disconnect(eDisconnectReason::ServiceError);
		return;
	}

	Register();
}

void Sender::Process(const uint32 numOfBytes)
{
	const SessionRef pOwner = static_pointer_cast<Session>(mSendEvent.GetOwner());
	if (pOwner == nullptr || pOwner->IsDisconnected())
	{
		return;
	}

	if (numOfBytes == 0)
	{
		pOwner->Disconnect(eDisconnectReason::Closed);
		return;
	}

	pOwner->OnSend(static_cast<int32>(numOfBytes));

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
		const SessionRef pOwner = static_pointer_cast<Session>(mSendEvent.GetOwner());
		if (pOwner == nullptr || pOwner->IsDisconnected())
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
		if (SocketUtils::WsaSend(pOwner->GetSocket(), wsabufs, sendCount, &mSendEvent) == false)
		{
			const int32 errorCode = WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				pOwner->OnError(errorCode);
				pOwner->Disconnect(eDisconnectReason::SocketError);
			}
		}

		return;
	}
}
