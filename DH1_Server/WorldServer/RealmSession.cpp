#include "pch.h"
#include "RealmSession.h"

RealmSession::RealmSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	: PacketSession(receiveBufferSize, maxPacketSize)
{
}

void RealmSession::OnConnected()
{
	NET_ENGINE_LOG_INFO("RealmSession::OnConnected - Connected to RealmServer, sessionId: {}", GetSessionId());
}

void RealmSession::OnDisconnecting(const eDisconnectReason reason)
{
	NET_ENGINE_LOG_INFO("RealmSession::OnDisconnecting - sessionId: {}, reason: {}", GetSessionId(), static_cast<int32>(reason));
}

void RealmSession::OnDisconnected()
{
	NET_ENGINE_LOG_INFO("RealmSession::OnDisconnected - Disconnected from RealmServer, sessionId: {}", GetSessionId());
}

void RealmSession::OnSend(const int32 len)
{
}

void RealmSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	// TODO: PacketServiceTypeHandler
}
