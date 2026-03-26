#include "pch.h"
#include "GameSessionPacketHandler.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "GatewayService.h"
#include "WorldPacketHandler.h"

bool GameSessionPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool GameSessionPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("GameSessionPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool GameSessionPacketHandler::HANDLE_S2S_GAME_SESSION_ENTER_RES(const Protocol::S2S_GAME_SESSION_ENTER_RES& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = packet.accountid();
	const auto result = packet.result();

	auto& gatewayService = ISingleton<GatewayService>::GetInstance();
	const auto pClientSessionManager = gatewayService.GetClientSessionManagerRef();
	if (pClientSessionManager == nullptr)
	{
		return false;
	}

	const auto pClientSession = pClientSessionManager->GetClientSession(accountId);
	if (pClientSession == nullptr)
	{
		NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Client session not found for ENTER_RES, accountId: {}", accountId);
		return true;
	}

	Protocol::S2C_WORLD_SELECT_RES response;

	if (result == Protocol::GAME_SESSION_SUCCESS)
	{
		pClientSession->SetWorldServerId(gatewayService.GetWorldServerId());
		response.set_result(Protocol::WORLD_SELECT_SUCCESS);
		NET_ENGINE_LOG_INFO("GameSessionPacketHandler - World enter success, accountId: {}", accountId);
	}
	else if (result == Protocol::GAME_SESSION_FAIL_FULL)
	{
		response.set_result(Protocol::WORLD_SELECT_FAIL_FULL);
		NET_ENGINE_LOG_WARN("GameSessionPacketHandler - World enter failed (full), accountId: {}", accountId);
	}
	else
	{
		response.set_result(Protocol::WORLD_SELECT_FAIL_NOT_FOUND);
		NET_ENGINE_LOG_ERROR("GameSessionPacketHandler - World enter failed, accountId: {}, result: {}", accountId, static_cast<int32>(result));
	}

	pClientSession->Send(WorldPacketHandler::MakeSendBuffer(response));

	return true;
}

bool GameSessionPacketHandler::HANDLE_S2S_RELAY_TO_CLIENT_NOT(const Protocol::S2S_RELAY_TO_CLIENT_NOT& packet, const PacketSessionRef& pSession)
{
	const uint64 gatewaySessionId = packet.gatewaysessionid();
	const auto& payload = packet.payload();

	if (payload.empty())
	{
		NET_ENGINE_LOG_WARN("GameSessionPacketHandler - Empty relay payload, gatewaySessionId: {}", gatewaySessionId);
		return true;
	}

	// TODO: gatewaySessionId로 클라이언트 세션을 조회하여 payload 전달 (확장 포인트)
	NET_ENGINE_LOG_TRACE("GameSessionPacketHandler - Relay to client, gatewaySessionId: {}, payloadSize: {}",
		gatewaySessionId, payload.size());

	return true;
}
