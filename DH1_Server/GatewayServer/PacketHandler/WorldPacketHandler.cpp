#include "pch.h"
#include "WorldPacketHandler.h"
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

	pRedisService->GetWorldServerInfoAsync(worldId, [pSession, worldId](const bool bFound, const RedisWorldServerInfo& info) -> void
		{
			Protocol::S2C_WORLD_SELECT_RES response;

			if (!bFound)
			{
				response.set_result(Protocol::WORLD_SELECT_FAIL_NOT_FOUND);
			}
			else if (info.status == 1)
			{
				response.set_result(Protocol::WORLD_SELECT_FAIL_MAINTENANCE);
			}
			else if (info.status == 2 || info.currentPlayers >= info.maxPlayers)
			{
				response.set_result(Protocol::WORLD_SELECT_FAIL_FULL);
			}
			else
			{
				response.set_result(Protocol::WORLD_SELECT_SUCCESS);
			}

			pSession->Send(WorldPacketHandler::MakeSendBuffer(response));

			NET_ENGINE_LOG_INFO("WorldPacketHandler - WORLD_SELECT_RES sent, worldId: {}, result: {}", worldId, static_cast<int32>(response.result()));
		});

	return true;
}
