#pragma once
#include <PacketSession.h>

class GatewaySession final : public PacketSession
{
public:

	explicit GatewaySession(const int32 receiveBufferSize, const int32 maxPacketSize);
	virtual ~GatewaySession() override = default;

	virtual void OnConnected() override;
	virtual void OnDisconnecting(const eDisconnectReason reason) override;
	virtual void OnDisconnected() override;
	virtual void OnSend(const int32 len) override;
	virtual void OnReceivePacket(const byte* pBuffer, const int32 len) override;
};
