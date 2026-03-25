#pragma once
#include <PacketSession.h>

class WorldSession final : public PacketSession
{
public:

	explicit WorldSession(const int32 receiveBufferSize, const int32 maxPacketSize);
	virtual ~WorldSession() override = default;

	virtual void OnConnected() override;
	virtual void OnDisconnecting(const eDisconnectReason reason) override;
	virtual void OnDisconnected() override;
	virtual void OnSend(const int32 len) override;
	virtual void OnReceivePacket(const byte* pBuffer, const int32 len) override;

	void UpdateHeartbeat(int32 serverId);
	[[nodiscard]] int64 GetLastHeartbeatMs() const;
	[[nodiscard]] int32 GetWorldServerId() const;

private:

	std::atomic<int64> mLastHeartbeatMs{0};
	int32 mWorldServerId = 0;
};
