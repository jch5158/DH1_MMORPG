#pragma once

class NetReceiveBuffer
{
public:

	static constexpr int32 DEFAULT_BUFFER_SIZE = 65535;

	NetReceiveBuffer(const NetReceiveBuffer&) = delete;
	NetReceiveBuffer& operator=(const NetReceiveBuffer&) = delete;
	NetReceiveBuffer(NetReceiveBuffer&&) = delete;
	NetReceiveBuffer& operator=(NetReceiveBuffer&&) = delete;

	explicit NetReceiveBuffer(const int32 maxSize = DEFAULT_BUFFER_SIZE);
	~NetReceiveBuffer() = default;

	void Clear();
	[[nodiscard]] bool IsEmpty() const;
	[[nodiscard]] int32 GetMaxSize() const;
	[[nodiscard]] int32 GetFreeSize() const;
	[[nodiscard]] int32 GetUseSize() const;
	[[nodiscard]] byte* GetBufferPtr() const;
	[[nodiscard]] byte* GetReadPtr() const;
	[[nodiscard]] byte * GetWritePtr() const;
	[[nodiscard]] int32 GetLinearWriteSize() const;
	[[nodiscard]] int32 GetLinearReadSize() const;
	void MoveReadPos(const int32 size);
	void MoveWritePos(const int32 size);
	int32 Write(const byte* pData, const int32 size);
	int32 Read(byte* pBuffer, const int32 size);
	int32 Peek(byte* pBuffer, const int32 size) const;
	void LinearizeRead();

private:
	const int32 mMaxBufferSize;
	int32 mReadPos;
	int32 mWritePos;
	UniquePtr<byte[]> mpBuffer;
};