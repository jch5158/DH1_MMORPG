#include "pch.h"
#include "LoginPacketHandler.h"
#include "ClientSession.h"
#include "GatewayService.h"

bool LoginPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
                                                  const PacketSessionRef& pSession)
{
	return false;
}

bool LoginPacketHandler::HANDLE_C2S_LOGIN_REQ(const Protocol::C2S_LOGIN_REQ& packet, const PacketSessionRef& pSession)
{
	const RedisServiceRef pRedisService = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
	if (pRedisService == nullptr)
	{
		return false;
	}

	pRedisService->GetStringAsync(packet.ticket(), [pSession, pRedisService, argTicket = packet.ticket(), argAccountId = packet.accountid()](const std::optional<std::string>& resultStr)->void
		{
			Protocol::S2C_LOGIN_RES retPacket;

			auto sendResponse = [&retPacket, &pSession](const Protocol::eLoginResult result) {
				retPacket.set_result(result);
				pSession->Send(MakeSendBuffer(retPacket));
				};

			if (!resultStr.has_value())
			{
				sendResponse(Protocol::eLoginResult::LOGIN_FAIL_INVALID_TICKET);
				return;
			}

			const std::string& accountId = resultStr.value();
			if (accountId != std::to_string(argAccountId))
			{
				sendResponse(Protocol::eLoginResult::LOGIN_FAIL_INVALID_TICKET);
				return;
			}

			const ClientSessionManagerRef pManager = ISingleton<GatewayService>::GetInstance().GetClientSessionManagerRef();
			if (pManager == nullptr)
			{
				sendResponse(Protocol::eLoginResult::LOGIN_FAIL_INTERNAL_ERROR);
				return;
			}

			if (const auto pClientSession = pManager->GetClientSession(argAccountId))
			{
				pClientSession->Disconnect(eDisconnectReason::DuplicateLogin);
				(void)pManager->RemoveClientSession(argAccountId);
			}

			if (pManager->AddClientSession(argAccountId, std::static_pointer_cast<ClientSession>(pSession)) == false)
			{
				sendResponse(Protocol::eLoginResult::LOGIN_FAIL_INTERNAL_ERROR);
				return;
			}

			pRedisService->DeleteKeyAsync(argTicket, [](bool) {});
			sendResponse(Protocol::eLoginResult::LOGIN_SUCCESS);
		});

	return true;
}
