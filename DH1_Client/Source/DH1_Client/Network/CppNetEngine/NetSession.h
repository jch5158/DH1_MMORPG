#pragma once
#include "CoreMinimal.h"
#include <PacketSession.h>
#include <Types.h>

class DH1_CLIENT_API NetSession : public PacketSession
{
public:
	NetSession(const int32 receiveBufferSize, const int32 maxPacketSize);
	virtual ~NetSession() override;
	virtual void OnConnected() override;
	virtual void OnDisconnecting(const eDisconnectReason reason) override;
	virtual void OnDisconnected() override;
	virtual void OnSend(const int32 len) override;
	virtual void OnError(const int32 errorCode) override;

	virtual void OnReceivePacket(const byte* pBuffer, const int32 len) override;
private:
};

DECLARE_SMART_PTR(NetSession);