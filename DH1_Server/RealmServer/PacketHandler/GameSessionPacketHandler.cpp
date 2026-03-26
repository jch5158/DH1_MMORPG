#include "pch.h"
#include "GameSessionPacketHandler.h"
#include "RealmSessionRegistry.h"
#include "RealmService.h"

bool GameSessionPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool GameSessionPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("GameSessionPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool GameSessionPacketHandler::HANDLE_S2S_GAME_SESSION_SYNC_ENTER_NOT(const Protocol::S2S_GAME_SESSION_SYNC_ENTER_NOT& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = packet.accountid();
	const int32 worldServerId = packet.worldserverid();
	const int32 gatewayServerId = packet.gatewayserverid();

	const auto pSessionRegistry = ISingleton<RealmService>::GetInstance().GetSessionRegistryRef();
	if (pSessionRegistry == nullptr)
	{
		return false;
	}

	RealmSessionInfo sessionInfo;
	sessionInfo.mAccountId = accountId;
	sessionInfo.mWorldServerId = worldServerId;
	sessionInfo.mGatewayServerId = gatewayServerId;

	if (pSessionRegistry->AddSession(sessionInfo))
	{
		NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Session synced (enter), accountId: {}, worldServerId: {}, gatewayServerId: {}, totalCount: {}",
			accountId, worldServerId, gatewayServerId, pSessionRegistry->GetTotalSessionCount());
	}
	else
	{
		NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Session sync failed (duplicate), accountId: {}", accountId);
	}

	return true;
}

bool GameSessionPacketHandler::HANDLE_S2S_GAME_SESSION_SYNC_LEAVE_NOT(const Protocol::S2S_GAME_SESSION_SYNC_LEAVE_NOT& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = packet.accountid();
	const int32 worldServerId = packet.worldserverid();

	const auto pSessionRegistry = ISingleton<RealmService>::GetInstance().GetSessionRegistryRef();
	if (pSessionRegistry == nullptr)
	{
		return false;
	}

	if (pSessionRegistry->RemoveSession(accountId))
	{
		NET_ENGINE_LOG_INFO("GameSessionPacketHandler - Session synced (leave), accountId: {}, worldServerId: {}, totalCount: {}",
			accountId, worldServerId, pSessionRegistry->GetTotalSessionCount());
	}
	else
	{
		NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Session not found for removal, accountId: {}", accountId);
	}

	return true;
}
