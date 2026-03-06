#include "pch.h"
#include "NetReceiveBuffer.h"

NetReceiveBuffer::NetReceiveBuffer(const int32 maxSize)
	: mMaxBufferSize(maxSize)
	, mReadPos(0)
	, mWritePos(0)
	, mpBuffer(cpp_net_engine::MakeUniqueArray<byte>(maxSize + 1))
{
}

void NetReceiveBuffer::Clear()
{
	mReadPos = 0;
	mWritePos = 0;
}

bool NetReceiveBuffer::IsEmpty() const
{
	return mReadPos == mWritePos;
}

int32 NetReceiveBuffer::GetMaxSize() const
{
	return mMaxBufferSize;
}

int32 NetReceiveBuffer::GetFreeSize() const
{
	const int32 readPos = mReadPos;
	const int32 writePos = mWritePos;
	int32 freeSize;

	if (readPos > writePos)
	{
		freeSize = readPos - writePos - 1;
	}
	else
	{
		freeSize = mMaxBufferSize - (writePos - readPos) - 1;
	}

	return freeSize;
}

int32 NetReceiveBuffer::GetUseSize() const
{
	const int32 readPos = mReadPos;
	const int32 writePos = mWritePos;
	int32 useSize;

	if (readPos > writePos)
	{
		useSize = mMaxBufferSize - (readPos - writePos);
	}
	else
	{
		useSize = writePos - readPos;
	}

	return useSize;
}

byte* NetReceiveBuffer::GetBufferPtr() const
{
	return &mpBuffer[0];
}

byte* NetReceiveBuffer::GetReadPtr() const
{
	return &mpBuffer[mReadPos];
}

byte* NetReceiveBuffer::GetWritePtr() const
{
	return &mpBuffer[mWritePos];
}

int32 NetReceiveBuffer::GetLinearWriteSize() const
{
	const int32 readPos = mReadPos;
	const int32 writePos = mWritePos;
	int32 writeSize;

	if (readPos > writePos)
	{
		writeSize = readPos - writePos - 1;
	}
	else
	{
		writeSize = mMaxBufferSize - writePos - (readPos == 0 ? 1 : 0);
	}

	return writeSize;
}

int32 NetReceiveBuffer::GetLinearReadSize() const
{
	const int32 readPos = mReadPos;
	const int32 writePos = mWritePos;
	int32 readSize;

	if (readPos > writePos)
	{
		readSize = mMaxBufferSize - readPos;
	}
	else
	{
		readSize = writePos - readPos;
	}

	return readSize;
}

void NetReceiveBuffer::MoveReadPos(const int32 size)
{
	mReadPos = (mReadPos + size) % mMaxBufferSize;
}

void NetReceiveBuffer::MoveWritePos(const int32 size)
{
	mWritePos = (mWritePos + size) % mMaxBufferSize;
}

int32 NetReceiveBuffer::Write(const byte* pData, const int32 size)
{
	const int32 writeSize = std::min(GetFreeSize(), size);
	if (writeSize == 0)
	{
		return writeSize;
	}

	if (writeSize + mWritePos < mMaxBufferSize)
	{
		std::copy_n(pData, writeSize, &mpBuffer[mWritePos]);
	}
	else
	{
		const int32 linearSize = GetLinearWriteSize();
		std::copy_n(pData, linearSize, &mpBuffer[mWritePos]);

		const int32 remainSize = writeSize - linearSize;
		std::copy_n(&pData[linearSize], remainSize, &mpBuffer[0]);
	}

	MoveWritePos(writeSize);

	return writeSize;
}

int32 NetReceiveBuffer::Read(byte* pBuffer, const int32 size)
{
	const int32 readSize = std::min(GetUseSize(), size);
	if (readSize == 0)
	{
		return readSize;
	}

	if (readSize + mReadPos > mMaxBufferSize)
	{
		const int32 linearSize = GetLinearReadSize();
		std::copy_n(&mpBuffer[mReadPos], linearSize, pBuffer);

		const int32 remainSize = readSize - linearSize;
		std::copy_n(&mpBuffer[0], remainSize, &pBuffer[linearSize]);
	}
	else
	{
		std::copy_n(&mpBuffer[mReadPos], readSize, pBuffer);
	}

	MoveReadPos(readSize);

	return readSize;
}

int32 NetReceiveBuffer::Peek(byte* pBuffer, const int32 size) const
{
	const int32 readSize = std::min(GetUseSize(), size);
	if (readSize == 0)
	{
		return readSize;
	}

	if (readSize + mReadPos > mMaxBufferSize)
	{
		const int32 linearSize = GetLinearReadSize();
		std::copy_n(&mpBuffer[mReadPos], linearSize, pBuffer);

		const int32 remainSize = readSize - linearSize;
		std::copy_n(&mpBuffer[0], remainSize, &pBuffer[linearSize]);
	}
	else
	{
		std::copy_n(&mpBuffer[mReadPos], readSize, pBuffer);
	}

	return readSize;
}
