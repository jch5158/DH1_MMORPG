#pragma once

class NetSendBuffer
{
public:

	static constexpr int32 MAX_BUFFER_SIZE = 655336;

	NetSendBuffer(const NetSendBuffer&) = delete;
	NetSendBuffer& operator=(const NetSendBuffer&) = delete;
	NetSendBuffer(NetSendBuffer&&) = delete;
	NetSendBuffer& operator=(NetSendBuffer&&) = delete;

	explicit NetSendBuffer(const int32 maxSize);
	~NetSendBuffer() = default;
	
	void Clear();

	[[nodiscard]] int32 GetMaxSize() const;
	[[nodiscard]] int32 GetFreeSize() const;
	[[nodiscard]] int32 GetUseSize() const;
	[[nodiscard]] byte* GetBufferPtr() const;
	[[nodiscard]] byte* GetReadPtr() const;
	[[nodiscard]] byte* GetWritePtr() const;
	[[nodiscard]] byte* Reserve(const int32 size) const;
	void Commit(const int32 size);
	void MoveReadPos(const int32 size);
	void MoveWritePos(const int32 size);

private:
	const int32 mMaxBufferSize;
	int32 mReadPos;
	int32 mWritePos;
	UniquePtr<byte[]> mpBuffer;
};
