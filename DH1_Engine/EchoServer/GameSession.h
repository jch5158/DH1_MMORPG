#pragma once
#include "PacketSession.h"

class GameSession final : public PacketSession
{
public:

	explicit GameSession(const int32 receiveBufferSize, const int32 maxPacketSize);
	virtual ~GameSession() override = default;

	virtual void OnConnected() override;
	virtual void OnDisconnecting(const eDisconnectReason reason) override;
	virtual void OnDisconnected() override;
	virtual void OnSend(const int32 len) override;
	virtual void OnReceivePacket(byte* pBuffer,const int32 len) override;
	virtual void OnError(const int32 errorCode) override;
};

