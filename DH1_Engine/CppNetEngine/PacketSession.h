#pragma once
#include "Session.h"
#include "SessionTimeoutTracker.h"

#pragma pack(push, 1)
struct PacketHeader
{
	uint16 size = 0;
	uint32 id = 0;
};
#pragma pack(pop)

class PacketSession : public Session
{
public:
	explicit PacketSession(const int32 receiveBufferSize, const int32 maxPacketSize);
	virtual ~PacketSession() override = default;

	PacketSessionRef GetPacketSessionRef();

	virtual int32 OnReceive(const byte* pBuffer, const int32 len) override final;
	virtual void OnReceivePacket(const byte* pBuffer, const int32 len) = 0;

private:
	const int32 mMaxPacketSize;
};

