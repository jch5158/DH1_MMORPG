#include "pch.h"
#include "WorldPacketHandler.h"
#include "GameSessionPacketHandler.h"
#include "ClientSession.h"
#include "GatewayService.h"
#include "RedisService.h"

bool WorldPacketHandler::Validate(const PacketSessionRef& pSession)
{
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	if (!pClientSession->IsLoggedIn())
	{
		return false;
	}

	return true;
}

bool WorldPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("WorldPacketHandler::HANDLE_PACKET_ID_INVALID - packetId: {}", packetId);
	return false;
}

bool WorldPacketHandler::HANDLE_C2S_WORLD_LIST_REQ(const Protocol::C2S_WORLD_LIST_REQ& packet, const PacketSessionRef& pSession)
{
	RedisServiceRef pRedisService = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
	if (pRedisService == nullptr)
	{
		return false;
	}

	pRedisService->GetWorldServerListAsync([pSession](const Vector<RedisWorldServerInfo>& worldServers) -> void
		{
			Protocol::S2C_WORLD_LIST_RES response;

			for (const auto& worldServer : worldServers)
			{
				auto* pWorld = response.add_worlds();
				pWorld->set_worldid(worldServer.worldId);
				pWorld->set_worldname(worldServer.worldName);
				pWorld->set_currentplayers(worldServer.currentPlayers);
				pWorld->set_maxplayers(worldServer.maxPlayers);
				pWorld->set_status(worldServer.status);
			}

			pSession->Send(WorldPacketHandler::MakeSendBuffer(response));

			NET_ENGINE_LOG_INFO("WorldPacketHandler - WORLD_LIST_RES sent, worldCount: {}", response.worlds_size());
		});

	return true;
}

bool WorldPacketHandler::HANDLE_C2S_WORLD_SELECT_REQ(const Protocol::C2S_WORLD_SELECT_REQ& packet, const PacketSessionRef& pSession)
{
	const int32 worldId = packet.worldid();

	RedisServiceRef pRedisService = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
	if (pRedisService == nullptr)
	{
		return false;
	}

	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	const uint64 accountId = pClientSession->GetAccountId();
	const uint64 gatewaySessionId = pClientSession->GetSessionId();

	pRedisService->GetWorldServerInfoAsync(worldId, [pSession, pClientSession, worldId, accountId, gatewaySessionId](const bool bFound, const RedisWorldServerInfo& info) -> void
		{
			// Redis 검증 실패 시 바로 클라이언트에 응답
			if (!bFound)
			{
				Protocol::S2C_WORLD_SELECT_RES response;
				response.set_result(Protocol::WORLD_SELECT_FAIL_NOT_FOUND);
				pSession->Send(WorldPacketHandler::MakeSendBuffer(response));
				NET_ENGINE_LOG_INFO("WorldPacketHandler - WORLD_SELECT_RES sent (not found), worldId: {}", worldId);
				return;
			}

			if (info.status == 1)
			{
				Protocol::S2C_WORLD_SELECT_RES response;
				response.set_result(Protocol::WORLD_SELECT_FAIL_MAINTENANCE);
				pSession->Send(WorldPacketHandler::MakeSendBuffer(response));
				NET_ENGINE_LOG_INFO("WorldPacketHandler - WORLD_SELECT_RES sent (maintenance), worldId: {}", worldId);
				return;
			}

			if (info.status == 2 || info.currentPlayers >= info.maxPlayers)
			{
				Protocol::S2C_WORLD_SELECT_RES response;
				response.set_result(Protocol::WORLD_SELECT_FAIL_FULL);
				pSession->Send(WorldPacketHandler::MakeSendBuffer(response));
				NET_ENGINE_LOG_INFO("WorldPacketHandler - WORLD_SELECT_RES sent (full), worldId: {}", worldId);
				return;
			}

			// Redis 검증 통과 → WorldServer에 세션 진입 요청
			auto& gatewayService = ISingleton<GatewayService>::GetInstance();
			const auto pWorldClientService = gatewayService.GetWorldClientServiceRef();
			if (pWorldClientService == nullptr)
			{
				Protocol::S2C_WORLD_SELECT_RES response;
				response.set_result(Protocol::WORLD_SELECT_FAIL_NOT_FOUND);
				pSession->Send(WorldPacketHandler::MakeSendBuffer(response));
				return;
			}

			const auto pWorldSession = pWorldClientService->GetFirstSessionRef();
			if (pWorldSession == nullptr)
			{
				Protocol::S2C_WORLD_SELECT_RES response;
				response.set_result(Protocol::WORLD_SELECT_FAIL_NOT_FOUND);
				pSession->Send(WorldPacketHandler::MakeSendBuffer(response));
				return;
			}

			Protocol::S2S_GAME_SESSION_ENTER_NOT enterPacket;
			enterPacket.set_accountid(accountId);
			enterPacket.set_gatewaysessionid(gatewaySessionId);
			enterPacket.set_gatewayserverid(gatewayService.GetGatewayId());

			pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(enterPacket));

			NET_ENGINE_LOG_INFO("WorldPacketHandler - GAME_SESSION_ENTER_NOT sent, worldId: {}, accountId: {}", worldId, accountId);
		});

	return true;
}
