#include "pch.h"
#include "NetSendBuffer.h"

NetSendBuffer::NetSendBuffer(const int32 maxSize)
	: mMaxBufferSize(maxSize)
	, mReadPos(0)
	, mWritePos(0)
	, mpBuffer(cpp_net_engine::MakeUniqueArray<byte>(maxSize))
{
}

void NetSendBuffer::Clear()
{
	mReadPos = 0;
	mWritePos = 0;
}

int32 NetSendBuffer::GetMaxSize() const
{
	return mMaxBufferSize;
}

int32 NetSendBuffer::GetFreeSize() const
{
	return mMaxBufferSize - mWritePos;
}

int32 NetSendBuffer::GetUseSize() const
{
	return mWritePos;
}

byte* NetSendBuffer::GetBufferPtr() const
{
	return &mpBuffer[0];
}

byte* NetSendBuffer::GetReadPtr() const
{
	return &mpBuffer[mReadPos];
}

byte* NetSendBuffer::GetWritePtr() const
{
	return &mpBuffer[mWritePos];
}

byte* NetSendBuffer::Reserve(const int32 size) const
{
	if (std::cmp_less(GetFreeSize(), size))
	{
		return nullptr;
	}

	byte test = mpBuffer[0];

	return &mpBuffer[mWritePos];
}

void NetSendBuffer::Commit(const int32 size)
{
	MoveWritePos(size);
}

void NetSendBuffer::MoveReadPos(const int32 size)
{
	if (size + mReadPos > mWritePos)
	{
		return;
	}

	mReadPos += size;
}

void NetSendBuffer::MoveWritePos(const int32 size)
{
	if (std::cmp_less(GetFreeSize(), size))
	{
		return;
	}

	mWritePos += size;
}
