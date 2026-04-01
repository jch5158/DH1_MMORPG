#include "pch.h"
#include "LoginPacketHandler.h"
#include "GameSessionPacketHandler.h"
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
	std::weak_ptr<PacketSession> weakSession = pSession;
	pRedisService->GetDelStringAsync("ticket:" + packet.ticket(), [weakSession, argAccountId = packet.accountid()](const std::optional<std::string>& resultStr)->void
		{
			auto pSession = weakSession.lock();
			if (pSession == nullptr || !pSession->IsConnected())
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

			// 같은 GW 내 중복 로그인 체크
			if (const auto pExistingSession = pManager->GetClientSession(argAccountId))
			{
				NET_ENGINE_LOG_INFO("LoginPacketHandler - Duplicate login (same GW), kicking accountId: {}", argAccountId);

				// 월드에 진입한 상태라면 WorldServer에 이탈 통보 (DuplicateLogin)
				if (pExistingSession->IsInWorld())
				{
					const auto pWorldClientService = ISingleton<GatewayService>::GetInstance().GetWorldClientServiceRef();
					if (pWorldClientService != nullptr)
					{
						const auto pWorldSession = pWorldClientService->GetFirstSessionRef();
						if (pWorldSession != nullptr)
						{
							Protocol::S2S_GAME_SESSION_LEAVE_NOT sessionLeavePacket;
							sessionLeavePacket.set_accountid(argAccountId);
							sessionLeavePacket.set_gatewaysessionid(pExistingSession->GetSessionId());
							sessionLeavePacket.set_reason(static_cast<int32>(Protocol::eSessionLeaveReason::SESSION_LEAVE_DUPLICATE_LOGIN));
							pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(sessionLeavePacket));
						}
					}
				}

				{
					Protocol::S2C_KICK_NOT kickPacket;
					kickPacket.set_reason("다른 기기에서 로그인되어 연결이 종료되었습니다.");
					pExistingSession->Send(LoginPacketHandler::MakeSendBuffer(kickPacket));
				}
				pExistingSession->SetDuplicateLoginHandled();
				pExistingSession->Disconnect(eDisconnectReason::DuplicateLogin);
				(void)pManager->RemoveClientSession(argAccountId);
			}
			else
			{
				// 다른 GW에 접속 중인지 Redis로 확인 → PUBLISH 킥
				RedisServiceRef pRedisCheck = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
				if (pRedisCheck != nullptr)
				{
					const int32 myGatewayId = ISingleton<GatewayService>::GetInstance().GetGatewayId();
					pRedisCheck->GetStringAsync("Session:" + std::to_string(argAccountId), [argAccountId, myGatewayId, pRedisCheck](const std::optional<std::string>& value)
						{
							if (!value.has_value())
							{
								return;
							}

							try
							{
								const auto json = nlohmann::json::parse(value.value());
								const int32 existingGwId = json.at("gatewayId").get<int32>();
								if (existingGwId != myGatewayId)
								{
									NET_ENGINE_LOG_INFO("LoginPacketHandler - Duplicate login (cross GW), publishing kick to GW:{}, accountId: {}", existingGwId, argAccountId);
									pRedisCheck->PublishKickAsync(existingGwId, argAccountId);
								}
							}
							catch (const std::exception&)
							{
								// 파싱 실패 — 무시
							}
						});
				}
			}

			const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
			if (pManager->AddClientSession(argAccountId, pClientSession) == false)
			{
				NET_ENGINE_LOG_ERROR("LoginPacketHandler - Failed to add session, accountId: {}", argAccountId);
				sendResponse(Protocol::eLoginResult::LOGIN_FAIL_INTERNAL_ERROR);
				return;
			}

			pClientSession->SetAccountId(argAccountId);

			// Redis: 세션 등록 (온라인 상태 = Redis 키 존재 여부)
			const int32 gatewayId = ISingleton<GatewayService>::GetInstance().GetGatewayId();
			const int64 sessionTtlSeconds = ISingleton<GatewayService>::GetInstance().GetSessionTtlSeconds();
			RedisServiceRef pRedisForSession = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
			if (pRedisForSession != nullptr)
			{
				RedisSessionInfo sessionInfo;
				sessionInfo.gatewayId = gatewayId;
				sessionInfo.sessionId = pClientSession->GetSessionId();
				sessionInfo.loginTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
					std::chrono::system_clock::now().time_since_epoch()).count();

				pRedisForSession->SetSessionAsync(argAccountId, sessionInfo, sessionTtlSeconds, [argAccountId](const bool isSuccess)
					{
						if (!isSuccess)
						{
							NET_ENGINE_LOG_ERROR("LoginPacketHandler - Redis session set failed, accountId: {}", argAccountId);
						}
					});
			}

			// DB: last_login = now (is_online 제거 — Redis가 관리)
			MySqlServiceRef pMySqlService = ISingleton<GatewayService>::GetInstance().GetAccountMySqlServiceRef();
			if (pMySqlService != nullptr)
			{
				pMySqlService->ExecuteAsync(argAccountId, [argAccountId](sqlpp::mysql::connection& db)
					{
						try
						{
							const db::Account account{};
							db(sqlpp::update(account)
								.set(account.lastLogin = std::chrono::system_clock::now())
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
