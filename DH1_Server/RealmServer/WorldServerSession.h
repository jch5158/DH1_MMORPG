#pragma once
#include <PacketSession.h>

class WorldServerSession final : public PacketSession
{
public:

	explicit WorldServerSession(const int32 receiveBufferSize, const int32 maxPacketSize);
	virtual ~WorldServerSession() override = default;

	virtual void OnConnected() override;
	virtual void OnDisconnecting(const eDisconnectReason reason) override;
	virtual void OnDisconnected() override;
	virtual void OnSend(const int32 len) override;
	virtual void OnReceivePacket(const byte* pBuffer, const int32 len) override;
};
