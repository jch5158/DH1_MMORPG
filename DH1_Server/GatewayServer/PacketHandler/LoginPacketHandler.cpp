#include "pch.h"
#include "LoginPacketHandler.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "RedisService.h"
#include "MySqlService.h"
#include "GatewayService.h"
#include "AccountTable.h"

bool LoginPacketHandler::Validate(const PacketSessionRef& pSession)
{
	// 로그인 패킷은 인증 전이므로 항상 통과
	return true;
}

bool LoginPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
                                                  const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("LoginPacketHandler::HANDLE_PACKET_ID_INVALID - packetId: {}", packetId);
	return false;
}

bool LoginPacketHandler::HANDLE_C2S_LOGIN_REQ(const Protocol::C2S_LOGIN_REQ& packet, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_INFO("LoginPacketHandler::HANDLE_C2S_LOGIN_REQ - accountId: {}", packet.accountid());

	if (packet.accountid() == 0)
	{
		NET_ENGINE_LOG_ERROR("LoginPacketHandler::HANDLE_C2S_LOGIN_REQ - Invalid accountId(0)");
		return false;
	}

	RedisServiceRef pRedisService = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
	if (pRedisService == nullptr)
	{
		NET_ENGINE_LOG_ERROR("LoginPacketHandler::HANDLE_C2S_LOGIN_REQ - RedisService is null");
		return false;
	}

	// GetDelStringAsync: Redis GETDEL로 조회와 삭제를 원자적으로 수행 (티켓 재사용 방지)
	pRedisService->GetDelStringAsync("ticket:" + packet.ticket(), [pSession, argAccountId = packet.accountid()](const std::optional<std::string>& resultStr)->void
		{
			if (!pSession->IsConnected())
			{
				return;
			}

			Protocol::S2C_LOGIN_RES retPacket;

			auto sendResponse = [&retPacket, &pSession](const Protocol::eLoginResult result) {
				retPacket.set_result(result);
				pSession->Send(MakeSendBuffer(retPacket));
				};

			if (!resultStr.has_value())
			{
				NET_ENGINE_LOG_ERROR("LoginPacketHandler - Ticket not found or expired, accountId: {}", argAccountId);
				sendResponse(Protocol::eLoginResult::LOGIN_FAIL_INVALID_TICKET);
				return;
			}

			const std::string& accountId = resultStr.value();
			if (accountId != std::to_string(argAccountId))
			{
				NET_ENGINE_LOG_ERROR("LoginPacketHandler - Ticket mismatch, expected: {}, got: {}", argAccountId, accountId);
				sendResponse(Protocol::eLoginResult::LOGIN_FAIL_INVALID_TICKET);
				return;
			}

			const ClientSessionManagerRef pManager = ISingleton<GatewayService>::GetInstance().GetClientSessionManagerRef();
			if (pManager == nullptr)
			{
				NET_ENGINE_LOG_ERROR("LoginPacketHandler - ClientSessionManager is null");
				sendResponse(Protocol::eLoginResult::LOGIN_FAIL_INTERNAL_ERROR);
				return;
			}

			if (const auto pExistingSession = pManager->GetClientSession(argAccountId))
			{
				NET_ENGINE_LOG_INFO("LoginPacketHandler - Duplicate login, kicking accountId: {}", argAccountId);
				pExistingSession->Disconnect(eDisconnectReason::DuplicateLogin);
				(void)pManager->RemoveClientSession(argAccountId);
			}

			const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
			if (pManager->AddClientSession(argAccountId, pClientSession) == false)
			{
				NET_ENGINE_LOG_ERROR("LoginPacketHandler - Failed to add session, accountId: {}", argAccountId);
				sendResponse(Protocol::eLoginResult::LOGIN_FAIL_INTERNAL_ERROR);
				return;
			}

			pClientSession->SetAccountId(argAccountId);

			// DB: is_online = true, last_login = now
			MySqlServiceRef pMySqlService = ISingleton<GatewayService>::GetInstance().GetAccountMySqlServiceRef();
			if (pMySqlService != nullptr)
			{
				pMySqlService->ExecuteAsync([argAccountId](sqlpp::mysql::connection& db)
					{
						try
						{
							const db::Account account{};
							db(sqlpp::update(account)
								.set(account.isOnline = true, account.lastLogin = std::chrono::system_clock::now())
								.where(account.accountId == static_cast<int64>(argAccountId)));
						}
						catch (const sqlpp::exception& e)
						{
							NET_ENGINE_LOG_ERROR("LoginPacketHandler - DB update failed, accountId: {}, error: {}", argAccountId, e.what());
						}
					});
			}

			NET_ENGINE_LOG_INFO("LoginPacketHandler - Login success, accountId: {}", argAccountId);
			sendResponse(Protocol::eLoginResult::LOGIN_SUCCESS);
		});

	return true;
}
