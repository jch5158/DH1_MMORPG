#pragma once
#include <PacketSession.h>

class RealmSession final : public PacketSession
{
public:

	explicit RealmSession(const int32 receiveBufferSize, const int32 maxPacketSize);
	virtual ~RealmSession() override = default;

	virtual void OnConnected() override;
	virtual void OnDisconnecting(const eDisconnectReason reason) override;
	virtual void OnDisconnected() override;
	virtual void OnSend(const int32 len) override;
	virtual void OnReceivePacket(const byte* pBuffer, const int32 len) override;

	void UpdateHeartbeat();
	[[nodiscard]] int64 GetLastHeartbeatMs() const;

private:

	std::atomic<int64> mLastHeartbeatMs{0};
};
