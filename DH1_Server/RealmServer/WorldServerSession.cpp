#include "pch.h"
#include "WorldServerSession.h"
#include "PacketHandler/PacketServiceTypeHandler.h"
#include "RealmSessionRegistry.h"
#include "RealmService.h"

WorldServerSession::WorldServerSession(const int32 receiveBufferSize, const int32 maxPacketSize)
	: PacketSession(receiveBufferSize, maxPacketSize)
	, mLastHeartbeatMs(0)
	, mWorldServerId(0)
	, mWorldSessionCount(0)
	, mWorldStatus(0)
{
}

void WorldServerSession::OnConnected()
{
	NET_ENGINE_LOG_INFO("WorldServerSession::OnConnected - WorldServer connected, sessionId: {}", GetSessionId());
}

void WorldServerSession::OnDisconnecting(const eDisconnectReason reason)
{
	NET_ENGINE_LOG_INFO("WorldServerSession::OnDisconnecting - sessionId: {}, reason: {}", GetSessionId(), static_cast<int32>(reason));
}

void WorldServerSession::OnDisconnected()
{
	NET_ENGINE_LOG_WARN("WorldServerSession::OnDisconnected - WorldServer disconnected, sessionId: {}, worldServerId: {}", GetSessionId(), mWorldServerId);

	// WorldServer 끊김 시 해당 월드에 속한 모든 세션 제거
	if (mWorldServerId != 0)
	{
		const auto pSessionRegistry = ISingleton<RealmService>::GetInstance().GetSessionRegistryRef();
		if (pSessionRegistry != nullptr)
		{
			const Vector<uint64> removedAccountIds = pSessionRegistry->RemoveSessionsByWorldServer(mWorldServerId);

			if (!removedAccountIds.empty())
			{
				NET_ENGINE_LOG_INFO("WorldServerSession::OnDisconnected - Removed {} sessions for worldServerId: {}", removedAccountIds.size(), mWorldServerId);
			}
		}
	}
}

void WorldServerSession::OnSend(const int32 len)
{
}

void WorldServerSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, GetPacketSessionRef());
}

void WorldServerSession::UpdateHeartbeat(const int32 serverId, const int32 sessionCount, const int32 status)
{
	mWorldServerId = serverId;
	mWorldSessionCount = sessionCount;
	mWorldStatus = status;
	mLastHeartbeatMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count(), std::memory_order_release);
}

int64 WorldServerSession::GetLastHeartbeatMs() const
{
	return mLastHeartbeatMs.load(std::memory_order_acquire);
}

int32 WorldServerSession::GetWorldServerId() const
{
	return mWorldServerId;
}
