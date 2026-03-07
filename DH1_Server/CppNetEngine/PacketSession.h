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
	explicit PacketSession() = default;
	virtual ~PacketSession() override = default;

	PacketSessionRef GetPacketSessionRef();

	virtual int32 OnReceive(byte* pBuffer, const int32 len) override final;
	virtual void OnReceivePacket(byte* pBuffer, const int32 len) = 0;
};

