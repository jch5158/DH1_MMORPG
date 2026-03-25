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

	void UpdateHeartbeat(int32 serverId, int32 sessionCount, int32 status);

	[[nodiscard]] int64 GetLastHeartbeatMs() const;
	[[nodiscard]] int32 GetWorldServerId() const;

private:

	std::atomic<int64> mLastHeartbeatMs;
	int32 mWorldServerId;
	int32 mWorldSessionCount;
	int32 mWorldStatus;
};
