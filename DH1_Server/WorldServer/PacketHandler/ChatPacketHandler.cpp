#include "pch.h"
#include "ChatPacketHandler.h"
#include "GameObject.h"
#include "GameTickProcessor.h"
#include "GridManager.h"
#include "PlayerObject.h"
#include "RealmSession.h"
#include "RelayContext.h"
#include "WorldService.h"

namespace
{
	NetSendBufferRef MakeS2cChatNotSendBuffer(const Protocol::S2C_CHAT_NOT& msg)
	{
		const uint16 dataSize = static_cast<uint16>(msg.ByteSizeLong());
		const uint16 packetSize = dataSize + static_cast<uint16>(sizeof(PacketHeader));

		auto sendBuffer = cpp_net_engine::MakeSendBuffer(packetSize);
		byte* pBuffer = sendBuffer->Reserve(packetSize);
		if (pBuffer == nullptr)
		{
			return nullptr;
		}

		auto* header = reinterpret_cast<PacketHeader*>(pBuffer);
		header->size = packetSize;
		header->id = (static_cast<uint32>(Protocol::eServiceType::SERVICE_TYPE_CHAT) << 16) |
			static_cast<uint32>(packet_id::S2C_CHAT_NOT);
		if (!msg.SerializeToArray(&header[1], dataSize))
		{
			return nullptr;
		}
		sendBuffer->Commit(packetSize);
		return sendBuffer;
	}
}

bool ChatPacketHandler::Validate(const PacketSessionRef& pSession)
{
	(void)pSession;
	return true;
}

bool ChatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("ChatPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool ChatPacketHandler::HANDLE_C2S_CHAT_REQ(const Protocol::C2S_CHAT_REQ& packet, const PacketSessionRef& pSession)
{
	(void)pSession;

	const uint64 accountId = RelayContext::GetAccountId();
	if (accountId == 0)
	{
		NET_ENGINE_LOG_WARN("ChatPacketHandler - C2S_CHAT_REQ, RelayContext accountId is 0 (relay thread/context?)");
		return false;
	}

	auto& worldService = ISingleton<WorldService>::GetInstance();
	const auto pGridManager = worldService.GetGridManagerRef();
	const auto pGameTickProcessor = worldService.GetGameTickProcessorRef();
	if (pGridManager == nullptr || pGameTickProcessor == nullptr)
	{
		return false;
	}

	const auto pObject = pGridManager->GetObjectByAccountId(accountId);
	if (pObject == nullptr || pObject->GetObjectType() != eGameObjectType::Player)
	{
		NET_ENGINE_LOG_WARN("ChatPacketHandler - C2S_CHAT_REQ, PlayerObject not found, accountId: {}", accountId);
		return true;
	}

	const auto pSender = std::static_pointer_cast<PlayerObject>(pObject);

	const int64 serverTimestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	Protocol::S2C_CHAT_NOT notify;
	notify.set_channel(packet.channel());
	notify.set_sender_account_id(accountId);
	notify.set_sender_display_name(pSender->GetDisplayName());
	notify.set_message(packet.message());
	notify.set_server_timestamp_ms(serverTimestampMs);

	const NetSendBufferRef inner = MakeS2cChatNotSendBuffer(notify);
	if (inner == nullptr)
	{
		NET_ENGINE_LOG_ERROR("ChatPacketHandler - S2C_CHAT_NOT serialize failed, accountId: {}", accountId);
		return false;
	}

	NET_ENGINE_LOG_WARN(
		"[CHAT] World C2S_CHAT_REQ accountId={} channel={} msg_len={}",
		accountId,
		static_cast<int32>(packet.channel()),
		static_cast<int32>(packet.message().size()));

	switch (packet.channel())
	{
	case Protocol::CHAT_CHANNEL_LOCAL:
	case Protocol::CHAT_CHANNEL_WORLD:
	{
		const int32 cellX = pGridManager->ComputeCellX(pSender->GetPositionX());
		const int32 cellY = pGridManager->ComputeCellY(pSender->GetPositionY());
		const auto nearby = pGridManager->GetObjectsInRange(cellX, cellY);

		for (const auto& pNearby : nearby)
		{
			if (pNearby == nullptr || pNearby->GetObjectType() != eGameObjectType::Player)
			{
				continue;
			}
			const auto pNearbyPlayer = std::static_pointer_cast<PlayerObject>(pNearby);
			if (pNearbyPlayer->GetAccountId() == accountId)
			{
				continue;
			}
			pGameTickProcessor->RelayChatToGatewayClient(pNearbyPlayer->GetGatewaySessionId(),
				pNearbyPlayer->GetGatewayServerId(), inner);
		}
		break;
	}
	case Protocol::CHAT_CHANNEL_REALM:
	{
		const auto pRealmClientService = worldService.GetRealmClientServiceRef();
		if (pRealmClientService != nullptr)
		{
			const auto pRealmSession = std::static_pointer_cast<RealmSession>(pRealmClientService->GetFirstSessionRef());
			if (pRealmSession != nullptr)
			{
				Protocol::S2S_REALM_CHAT_SUBMIT_NOT submit;
				submit.set_sender_account_id(accountId);
				submit.set_sender_display_name(pSender->GetDisplayName());
				submit.set_message(packet.message());
				submit.set_server_timestamp_ms(serverTimestampMs);
				pRealmSession->Send(ChatPacketHandler::MakeSendBuffer(submit));
			}
		}

		for (const auto& pGo : pGridManager->GetAllObjects())
		{
			if (pGo == nullptr || pGo->GetObjectType() != eGameObjectType::Player)
			{
				continue;
			}
			const auto pPl = std::static_pointer_cast<PlayerObject>(pGo);
			if (pPl->GetAccountId() == accountId)
			{
				continue;
			}
			pGameTickProcessor->RelayChatToGatewayClient(pPl->GetGatewaySessionId(), pPl->GetGatewayServerId(), inner);
		}
		break;
	}
	default:
		NET_ENGINE_LOG_WARN("ChatPacketHandler - Unknown chat channel: {}", static_cast<int32>(packet.channel()));
		break;
	}

	return true;
}

bool ChatPacketHandler::HANDLE_S2S_REALM_CHAT_DELIVER_NOT(const Protocol::S2S_REALM_CHAT_DELIVER_NOT& packet, const PacketSessionRef& pSession)
{
	(void)pSession;

	const auto& payloadBytes = packet.s2c_payload();
	if (payloadBytes.empty())
	{
		NET_ENGINE_LOG_WARN("ChatPacketHandler - S2S_REALM_CHAT_DELIVER_NOT empty payload");
		return true;
	}

	const uint16 payloadSize = static_cast<uint16>(payloadBytes.size());
	auto innerBuffer = cpp_net_engine::MakeSendBuffer(payloadSize);
	byte* pBuf = innerBuffer->Reserve(payloadSize);
	if (pBuf == nullptr)
	{
		return false;
	}
	std::memcpy(pBuf, payloadBytes.data(), payloadSize);
	innerBuffer->Commit(payloadSize);

	const auto pGameTickProcessor = ISingleton<WorldService>::GetInstance().GetGameTickProcessorRef();
	if (pGameTickProcessor == nullptr)
	{
		return false;
	}

	for (int i = 0; i < packet.targets_size(); ++i)
	{
		const auto& t = packet.targets(i);
		pGameTickProcessor->RelayChatToGatewayClient(t.gateway_session_id(), t.gateway_server_id(), innerBuffer);
	}

	NET_ENGINE_LOG_WARN(
		"[CHAT] World S2S_REALM_CHAT_DELIVER_NOT target_count={} payload_len={}",
		packet.targets_size(),
		static_cast<int32>(payloadBytes.size()));

	return true;
}
