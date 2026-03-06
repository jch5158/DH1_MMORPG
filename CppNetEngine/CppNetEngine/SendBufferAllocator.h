#pragma once

#include "SharedPtrUtils.h"
#include "NetSendBuffer.h"
#include <tuple>
#include <utility> // std::integer_sequence
#include <array>

class SendBufferAllocator
{
private:

	static constexpr int32 SMALL_STRIDE = 256;
	static constexpr int32 LARGE_STRIDE = 4096;
	static constexpr int32 THRESHOLD = 4096;
	static constexpr int32 MAX_SIZE = 65536;

public:

	SendBufferAllocator() = delete;
	~SendBufferAllocator() = default;

	SendBufferAllocator(const SendBufferAllocator&) = delete;
	SendBufferAllocator& operator=(const SendBufferAllocator&) = delete;
	SendBufferAllocator(SendBufferAllocator&&) = delete;
	SendBufferAllocator& operator=(SendBufferAllocator&&) = delete;

	[[nodiscard]]
	static NetSendBufferRef Alloc(const int32 size);

private:

	[[nodiscard]] static int32 getAllocSize (const int32 size);
};

