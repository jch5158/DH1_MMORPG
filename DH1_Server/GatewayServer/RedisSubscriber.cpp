#include "pch.h"
#include "RedisSubscriber.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "GatewayService.h"
#include "PacketHandler/GameSessionPacketHandler.h"
#include "ThreadManager.h"

void RedisSubscriber::Start(const std::string& connectionUri, const int32 gatewayId)
{
	mConnectionUri = connectionUri;
	mGatewayId = gatewayId;
	mbRunning.store(true);

	ThreadManager::GetInstance().Launch("Redis Subscribe", [this]()->void
		{
			subscriberLoop();
		});

	NET_ENGINE_LOG_INFO("RedisSubscriber::Start - Subscribed to GW:kick:{}", mGatewayId);
}

void RedisSubscriber::Stop()
{
	mbRunning.store(false);

	// unsubscribe를 위해 별도 Redis 연결로 빈 메시지 발행하여 블로킹 해제
	try
	{
		if (mRedis != nullptr)
		{
			mRedis->publish("GW:kick:" + std::to_string(mGatewayId), "__shutdown__");
		}
	}
	catch (const sw::redis::Error&)
	{
		// 종료 중 에러 무시
	}

	NET_ENGINE_LOG_INFO("RedisSubscriber::Stop - Unsubscribed from GW:kick:{}", mGatewayId);
}

void RedisSubscriber::subscriberLoop()
{
	try
	{
		const sw::redis::Uri uri(mConnectionUri);
		sw::redis::ConnectionOptions options = uri.connection_options();
		sw::redis::ConnectionPoolOptions poolOptions = uri.connection_pool_options();

		mRedis = std::make_unique<sw::redis::Redis>(options, poolOptions);

		auto subscriber = mRedis->subscriber();

		subscriber.on_message([this](const std::string& /*channel*/, const std::string& message)->void
			{
				if (!mbRunning.load())
				{
					return;
				}

				if (message == "__shutdown__")
				{
					return;
				}

				onKickMessage(message);
			});

		const std::string channel = "GW:kick:" + std::to_string(mGatewayId);
		subscriber.subscribe(channel);

		while (mbRunning.load())
		{
			try
			{
				subscriber.consume();
			}
			catch (const sw::redis::TimeoutError&)
			{
				// 타임아웃은 정상 — 다시 루프
				continue;
			}
		}

		subscriber.unsubscribe(channel);
	}
	catch (const sw::redis::Error& e)
	{
		NET_ENGINE_LOG_ERROR("RedisSubscriber::subscriberLoop - Error: {}", e.what());
	}
}

void RedisSubscriber::onKickMessage(const std::string& message)
{
	try
	{
		const auto json = nlohmann::json::parse(message);
		const uint64 accountId = json.at("accountId").get<uint64>();

		NET_ENGINE_LOG_INFO("RedisSubscriber::onKickMessage - Kick request received, accountId: {}", accountId);

		const ClientSessionManagerRef pManager = ISingleton<GatewayService>::GetInstance().GetClientSessionManagerRef();
		if (pManager == nullptr)
		{
			return;
		}

		const auto pExistingSession = pManager->GetClientSession(accountId);
		if (pExistingSession == nullptr)
		{
			NET_ENGINE_LOG_INFO("RedisSubscriber::onKickMessage - Session not found, accountId: {}", accountId);
			return;
		}

		// 월드에 진입한 상태라면 WorldServer에 이탈 통보 (reason=DuplicateLogin)
		if (pExistingSession->IsInWorld())
		{
			const auto pWorldClientService = ISingleton<GatewayService>::GetInstance().GetWorldClientServiceRef();
			if (pWorldClientService != nullptr)
			{
				const auto pWorldSession = pWorldClientService->GetFirstSessionRef();
				if (pWorldSession != nullptr)
				{
					Protocol::S2S_GAME_SESSION_LEAVE_NOT sessionLeavePacket;
					sessionLeavePacket.set_accountid(accountId);
					sessionLeavePacket.set_gatewaysessionid(pExistingSession->GetSessionId());
					sessionLeavePacket.set_reason(static_cast<int32>(Protocol::eSessionLeaveReason::SESSION_LEAVE_DUPLICATE_LOGIN));

					pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(sessionLeavePacket));

					NET_ENGINE_LOG_INFO("RedisSubscriber::onKickMessage - LEAVE_NOT(DuplicateLogin) sent, accountId: {}", accountId);
				}
			}
		}

		// 세션 킥 (OnDisconnected에서 LEAVE_NOT 중복 전송 방지)
		pExistingSession->SetDuplicateLoginHandled();
		pExistingSession->Disconnect(eDisconnectReason::DuplicateLogin);
		(void)pManager->RemoveClientSession(accountId);

		NET_ENGINE_LOG_INFO("RedisSubscriber::onKickMessage - Session kicked, accountId: {}", accountId);
	}
	catch (const std::exception& e)
	{
		NET_ENGINE_LOG_ERROR("RedisSubscriber::onKickMessage - Parse error: {}", e.what());
	}
}
