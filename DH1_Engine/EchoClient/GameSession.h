#pragma once
#include "PacketSession.h"

class GameSession final : public PacketSession
{
public:

	static constexpr const char* ECHO_TEST_MESSAGE =
		"[StressTest] The quick brown fox jumps over the lazy dog. "
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 !@#$%^&*() "
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
		"Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.";

	explicit GameSession(const int32 receiveBufferSize, const int32 maxPacketSize);
	virtual ~GameSession() override = default;

	virtual void OnConnected() override;
	virtual void OnDisconnecting(const eDisconnectReason reason) override;
	virtual void OnDisconnected() override;
	virtual void OnSend(const int32 len) override;
	virtual void OnReceivePacket(const byte* pBuffer, const int32 len) override;
	virtual void OnError(const int32 errorCode) override;

private:
	friend class EchoPacketHandler;
	std::atomic<bool> mbDisconnectRequested;
};
