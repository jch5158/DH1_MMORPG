#pragma once
#include "CoreMinimal.h"
#include "Network/CppNetEngine/NetEngineWrapper.h"
#include <PacketSession.h>
#include <atomic>
#include <chrono>

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

	int64 GetLastRecvTimeMs() const { return mLastRecvTimeMs.load(std::memory_order_relaxed); }

	void MarkLocalDisconnect() { mLocalDisconnect.store(true, std::memory_order_relaxed); }

private:
	static int64 SteadyNowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	AuthData mAuthData;
	eDisconnectReason mbDisconnectReason = eDisconnectReason::Closed;
	std::atomic<int64> mLastRecvTimeMs{ 0 };
	std::atomic<bool> mLocalDisconnect{ false };
};

DECLARE_SMART_PTR(NetSession);