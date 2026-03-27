#include "pch.h"
#include "RealmPacketHandler.h"
#include "GameSessionPacketHandler.h"
#include "ClientSession.h"
#include "GatewayService.h"
#include "RedisService.h"

bool RealmPacketHandler::Validate(const PacketSessionRef& pSession)
{
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	if (!pClientSession->IsLoggedIn())
	{
		return false;
	}

	return true;
}

bool RealmPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("RealmPacketHandler::HANDLE_PACKET_ID_INVALID - packetId: {}", packetId);
	return false;
}

bool RealmPacketHandler::HANDLE_C2S_REALM_LIST_REQ(const Protocol::C2S_REALM_LIST_REQ& packet, const PacketSessionRef& pSession)
{
	RedisServiceRef pRedisService = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
	if (pRedisService == nullptr)
	{
		return false;
	}

	pRedisService->GetRealmListAsync([pSession](const Vector<RedisRealmInfo>& realms) -> void
		{
			Protocol::S2C_REALM_LIST_RES response;

			for (const auto& realm : realms)
			{
				auto* pRealm = response.add_realms();
				pRealm->set_realmid(realm.realmId);
				pRealm->set_realmname(realm.realmName);
				pRealm->set_currentplayers(realm.currentPlayers);
				pRealm->set_maxplayers(realm.maxPlayers);
				pRealm->set_status(realm.status);
			}

			pSession->Send(RealmPacketHandler::MakeSendBuffer(response));

			NET_ENGINE_LOG_INFO("RealmPacketHandler - REALM_LIST_RES sent, realmCount: {}", response.realms_size());
		});

	return true;
}

bool RealmPacketHandler::HANDLE_C2S_REALM_SELECT_REQ(const Protocol::C2S_REALM_SELECT_REQ& packet, const PacketSessionRef& pSession)
{
	const int32 realmId = packet.realmid();

	RedisServiceRef pRedisService = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
	if (pRedisService == nullptr)
	{
		return false;
	}

	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	const uint64 accountId = pClientSession->GetAccountId();
	const uint64 gatewaySessionId = pClientSession->GetSessionId();

	pRedisService->GetRealmInfoAsync(realmId, [pSession, pClientSession, realmId, accountId, gatewaySessionId](const bool bFound, const RedisRealmInfo& info) -> void
		{
			// Redis 검증 실패 시 바로 클라이언트에 응답
			if (!bFound)
			{
				Protocol::S2C_REALM_SELECT_RES response;
				response.set_result(Protocol::REALM_SELECT_FAIL_NOT_FOUND);
				pSession->Send(RealmPacketHandler::MakeSendBuffer(response));
				NET_ENGINE_LOG_INFO("RealmPacketHandler - REALM_SELECT_RES sent (not found), realmId: {}", realmId);
				return;
			}

			if (info.status == 1)
			{
				Protocol::S2C_REALM_SELECT_RES response;
				response.set_result(Protocol::REALM_SELECT_FAIL_MAINTENANCE);
				pSession->Send(RealmPacketHandler::MakeSendBuffer(response));
				NET_ENGINE_LOG_INFO("RealmPacketHandler - REALM_SELECT_RES sent (maintenance), realmId: {}", realmId);
				return;
			}

			if (info.status == 2 || info.currentPlayers >= info.maxPlayers)
			{
				Protocol::S2C_REALM_SELECT_RES response;
				response.set_result(Protocol::REALM_SELECT_FAIL_FULL);
				pSession->Send(RealmPacketHandler::MakeSendBuffer(response));
				NET_ENGINE_LOG_INFO("RealmPacketHandler - REALM_SELECT_RES sent (full), realmId: {}", realmId);
				return;
			}

			// Redis 검증 통과 -> WorldServer에 세션 진입 요청
			auto& gatewayService = ISingleton<GatewayService>::GetInstance();
			const auto pWorldClientService = gatewayService.GetWorldClientServiceRef();
			if (pWorldClientService == nullptr)
			{
				Protocol::S2C_REALM_SELECT_RES response;
				response.set_result(Protocol::REALM_SELECT_FAIL_NOT_FOUND);
				pSession->Send(RealmPacketHandler::MakeSendBuffer(response));
				return;
			}

			const auto pWorldSession = pWorldClientService->GetFirstSessionRef();
			if (pWorldSession == nullptr)
			{
				Protocol::S2C_REALM_SELECT_RES response;
				response.set_result(Protocol::REALM_SELECT_FAIL_NOT_FOUND);
				pSession->Send(RealmPacketHandler::MakeSendBuffer(response));
				return;
			}

			Protocol::S2S_GAME_SESSION_ENTER_NOT enterPacket;
			enterPacket.set_accountid(accountId);
			enterPacket.set_gatewaysessionid(gatewaySessionId);
			enterPacket.set_gatewayserverid(gatewayService.GetGatewayId());

			pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(enterPacket));

			NET_ENGINE_LOG_INFO("RealmPacketHandler - GAME_SESSION_ENTER_NOT sent, realmId: {}, accountId: {}", realmId, accountId);
		});

	return true;
}
