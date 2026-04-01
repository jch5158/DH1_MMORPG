#include "pch.h"
#include "ChatPacketHandler.h"
#include "ClientSession.h"
#include "GameSessionPacketHandler.h"
#include "GatewayService.h"

bool ChatPacketHandler::Validate(const PacketSessionRef& pSession)
{
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	return pClientSession->IsLoggedIn() && pClientSession->IsInWorld();
}

bool ChatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("ChatPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool ChatPacketHandler::HANDLE_C2S_CHAT_REQ(const Protocol::C2S_CHAT_REQ& packet, const PacketSessionRef& pSession)
{
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	const uint64 accountId = pClientSession->GetAccountId();

	auto& gatewayService = ISingleton<GatewayService>::GetInstance();
	const auto pWorldClientService = gatewayService.GetWorldClientServiceRef();
	if (pWorldClientService == nullptr)
	{
		return true;
	}

	const auto pWorldSession = pWorldClientService->GetFirstSessionRef();
	if (pWorldSession == nullptr)
	{
		return true;
	}

	const uint16 dataSize = static_cast<uint16>(packet.ByteSizeLong());
	const uint16 packetSize = dataSize + static_cast<uint16>(sizeof(PacketHeader));

	auto innerBuffer = cpp_net_engine::MakeSendBuffer(packetSize);
	byte* pBuffer = innerBuffer->Reserve(packetSize);
	if (pBuffer == nullptr)
	{
		return false;
	}

	auto* header = reinterpret_cast<PacketHeader*>(pBuffer);
	header->size = packetSize;
	header->id = ChatPacketHandler::MAKE_PACKET_HEADER_ID(
		Protocol::eServiceType::SERVICE_TYPE_CHAT, packet_id::C2S_CHAT_REQ);
	packet.SerializeToArray(&header[1], dataSize);
	innerBuffer->Commit(packetSize);

	Protocol::S2S_RELAY_TO_WORLD_NOT relay;
	relay.set_accountid(accountId);
	relay.set_payload(innerBuffer->GetReadPtr(), innerBuffer->GetUseSize());

	pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));

	NET_ENGINE_LOG_INFO(
		"[CHAT] Gateway relayed C2S_CHAT_REQ accountId={} channel={} msg_len={}",
		accountId,
		static_cast<int32>(packet.channel()),
		static_cast<int32>(packet.message().size()));

	return true;
}
