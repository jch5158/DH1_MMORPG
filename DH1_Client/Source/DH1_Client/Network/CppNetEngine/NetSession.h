#pragma once
#include "CoreMinimal.h"
#include "Network/CppNetEngine/NetEngineWrapper.h"
#include <PacketSession.h>
#include <atomic>

struct AuthData
{
	FString Ticket;
	FString AccountId;
};

class DH1_CLIENT_API NetSession : public PacketSession
{
public:
	NetSession(const int32 receiveBufferSize, const int32 maxPacketSize, AuthData ArgAuthData);
	virtual ~NetSession() override;
	virtual void OnConnected() override;
	virtual void OnDisconnecting(const eDisconnectReason reason) override;
	virtual void OnDisconnected() override;
	virtual void OnSend(const int32 len) override;

	virtual void OnReceivePacket(const byte* pBuffer, const int32 len) override;

	void MarkLocalDisconnect() { mLocalDisconnect.store(true, std::memory_order_relaxed); }

private:
	AuthData mAuthData;
	eDisconnectReason mbDisconnectReason = eDisconnectReason::Closed;
	std::atomic<bool> mLocalDisconnect{ false };
};

DECLARE_SMART_PTR(NetSession);