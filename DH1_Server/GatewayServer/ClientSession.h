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

	void SetWorldServerId(const int32 worldServerId) { mWorldServerId = worldServerId; }
	[[nodiscard]] int32 GetWorldServerId() const { return mWorldServerId; }
	[[nodiscard]] bool IsInWorld() const { return mWorldServerId != 0; }

	void UpdateHeartbeat();
	[[nodiscard]] int64 GetLastHeartbeatMs() const;

	void SetDuplicateLoginHandled() { mbDuplicateLoginHandled = true; }
	[[nodiscard]] bool IsDuplicateLoginHandled() const { return mbDuplicateLoginHandled; }

	// 이동 패킷 검증용
	void SetLastMoveSequenceId(const uint32 seq) { mLastMoveSequenceId = seq; }
	[[nodiscard]] uint32 GetLastMoveSequenceId() const { return mLastMoveSequenceId; }
	void SetLastMoveTimestampMs(const int64 ms) { mLastMoveTimestampMs = ms; }
	[[nodiscard]] int64 GetLastMoveTimestampMs() const { return mLastMoveTimestampMs; }

private:
	uint64 mAccountId = 0;
	int32 mWorldServerId = 0;
	bool mbDuplicateLoginHandled = false;
	std::atomic<int64> mLastHeartbeatMs{0};

	// 이동 패킷 검증
	uint32 mLastMoveSequenceId = 0;
	int64 mLastMoveTimestampMs = 0;
};
