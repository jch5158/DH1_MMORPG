#include "pch.h"
#include "LoginPacketHandler.h"

#include "GatewayService.h"

bool LoginPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
                                                  PacketSessionRef& pSession)
{
	return false;
}

bool LoginPacketHandler::HANDLE_C2S_LOGIN_REQ(const Protocol::C2S_LOGIN_REQ& packet, PacketSessionRef& pSession)
{
	const RedisServiceRef pRedisService = ISingleton<GatewayService>::GetInstance().GetRedisServiceRef();
	if (pRedisService == nullptr)
	{
		return false;
	}

	pRedisService->GetStringAsync(packet.ticket(), [pSession, argAccountId = packet.accountid()](const std::optional<std::string>& resultStr)->void
		{
			Protocol::S2C_LOGIN_RES retPacket;

			if (!resultStr.has_value())
			{
				// TODO : Failed;
				return;
			}

			const std::string& accountId = resultStr.value();
			if (accountId != std::to_string(argAccountId))
			{
				// TODO : Failed;
				return;
			}

			retPacket.set_result(Protocol::eLoginResult::LOGIN_SUCCESS);
			const auto sendBuffer = MakeSendBuffer(retPacket);
			pSession->Send(sendBuffer);
		});

	return true;
}
