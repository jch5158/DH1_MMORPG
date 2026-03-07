#include "pch.h"
#include "PacketSession.h"

PacketSession::PacketSession()
	: mTimeoutTracker()
{
}

PacketSessionRef PacketSession::GetPacketSessionRef()
{
	return static_pointer_cast<PacketSession>(shared_from_this());
}

int32 PacketSession::OnReceive(byte* pBuffer, const int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		const int32 dataSize = len - processLen;
		if (std::cmp_less(dataSize, SIZE_OF_16(PacketHeader)))
		{
			break;
		}

		auto [size, id] = *(reinterpret_cast<PacketHeader*>(&pBuffer[processLen]));
		if (std::cmp_less(size, SIZE_OF_16(PacketHeader)) || std::cmp_less(NetReceiveBuffer::DEFAULT_BUFFER_SIZE, size))
		{
			return -1;
		}

		if (std::cmp_less(dataSize, size))
		{
			break;
		}

		updateLastActivityMs();
		OnReceivePacket(&pBuffer[processLen], size);

		processLen += size;
	}

	return processLen;
}

void PacketSession::updateLastActivityMs()
{
	mTimeoutTracker.UpdateLastActivityMs();
}

int64 PacketSession::getLastActivityMs() const
{
	return mTimeoutTracker.GetLastActivityMs();
}
