#pragma once
#include <PacketSession.h>

class ClientSession final : public PacketSession
{
public:

	ClientSession(const int32 receiveBufferSize, const int32 maxPacketSize);
	virtual ~ClientSession() override = default;

	virtual void OnConnected() override;
	virtual void OnDisconnecting(const eDisconnectReason reason) override;
	virtual void OnDisconnected() override;
	virtual void OnSend(const int32 len) override;
	virtual void OnReceivePacket(const byte* pBuffer, const int32 len) override;

	void SetAccountId(const uint64 accountId) { mAccountId = accountId; }
	[[nodiscard]] uint64 GetAccountId() const { return mAccountId; }
	[[nodiscard]] bool IsLoggedIn() const { return mAccountId != 0; }

private:
	uint64 mAccountId = 0;
};
