#include "pch.h"
#include "MovementPacketHandler.h"
#include "GameSessionPacketHandler.h"
#include "ClientSession.h"
#include "GatewayService.h"

bool MovementPacketHandler::Validate(const PacketSessionRef& pSession)
{
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	if (!pClientSession->IsLoggedIn() || !pClientSession->IsInWorld())
	{
		return false;
	}

	return true;
}

bool MovementPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	NET_ENGINE_LOG_ERROR("MovementPacketHandler - Invalid packetId: {}", packetId);
	return false;
}

bool MovementPacketHandler::HANDLE_C2S_MOVE_INPUT_NOT(const Protocol::C2S_MOVE_INPUT_NOT& packet, const PacketSessionRef& pSession)
{
	// WASD 이동 비활성화 — 클릭이동으로 통일
	return true;
}

bool MovementPacketHandler::HANDLE_C2S_SPAWN_POSITION_REQ(const Protocol::C2S_SPAWN_POSITION_REQ& packet, const PacketSessionRef& pSession)
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

	// 빈 패킷이므로 헤더만 전송
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
	header->id = MovementPacketHandler::MAKE_PACKET_HEADER_ID(
		Protocol::eServiceType::SERVICE_TYPE_MOVEMENT, packet_id::C2S_SPAWN_POSITION_REQ);
	packet.SerializeToArray(&header[1], dataSize);
	innerBuffer->Commit(packetSize);

	Protocol::S2S_RELAY_TO_WORLD_NOT relay;
	relay.set_accountid(accountId);
	relay.set_payload(innerBuffer->GetReadPtr(), innerBuffer->GetUseSize());

	pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));

	NET_ENGINE_LOG_INFO("MovementPacketHandler - SPAWN_POSITION_REQ relayed, accountId: {}", accountId);

	return true;
}

bool MovementPacketHandler::HANDLE_C2S_MOVE_TO_POSITION_REQ(const Protocol::C2S_MOVE_TO_POSITION_REQ& packet, const PacketSessionRef& pSession)
{
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	const uint64 accountId = pClientSession->GetAccountId();

	// 목적지 NaN/Inf 검증
	const float destX = packet.destination().x();
	const float destY = packet.destination().y();
	const float destZ = packet.destination().z();

	if (std::isnan(destX) || std::isnan(destY) || std::isnan(destZ) ||
		std::isinf(destX) || std::isinf(destY) || std::isinf(destZ))
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler - Invalid destination, accountId: {}", accountId);
		return false;
	}

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
	header->id = MovementPacketHandler::MAKE_PACKET_HEADER_ID(
		Protocol::eServiceType::SERVICE_TYPE_MOVEMENT, packet_id::C2S_MOVE_TO_POSITION_REQ);
	packet.SerializeToArray(&header[1], dataSize);
	innerBuffer->Commit(packetSize);

	Protocol::S2S_RELAY_TO_WORLD_NOT relay;
	relay.set_accountid(accountId);
	relay.set_payload(innerBuffer->GetReadPtr(), innerBuffer->GetUseSize());

	pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));

	NET_ENGINE_LOG_TRACE("MovementPacketHandler - MOVE_TO_POSITION_REQ relayed, accountId: {}", accountId);

	return true;
}

bool MovementPacketHandler::HANDLE_C2S_CREATE_CHARACTER_REQ(const Protocol::C2S_CREATE_CHARACTER_REQ& packet, const PacketSessionRef& pSession)
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
	header->id = MovementPacketHandler::MAKE_PACKET_HEADER_ID(
		Protocol::eServiceType::SERVICE_TYPE_MOVEMENT, packet_id::C2S_CREATE_CHARACTER_REQ);
	packet.SerializeToArray(&header[1], dataSize);
	innerBuffer->Commit(packetSize);

	Protocol::S2S_RELAY_TO_WORLD_NOT relay;
	relay.set_accountid(accountId);
	relay.set_payload(innerBuffer->GetReadPtr(), innerBuffer->GetUseSize());

	pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));

	NET_ENGINE_LOG_INFO("MovementPacketHandler - CREATE_CHARACTER_REQ relayed, accountId: {}", accountId);

	return true;
}

bool MovementPacketHandler::HANDLE_C2S_JUMP_NOT(const Protocol::C2S_JUMP_NOT& packet, const PacketSessionRef& pSession)
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
	header->id = MovementPacketHandler::MAKE_PACKET_HEADER_ID(
		Protocol::eServiceType::SERVICE_TYPE_MOVEMENT, packet_id::C2S_JUMP_NOT);
	packet.SerializeToArray(&header[1], dataSize);
	innerBuffer->Commit(packetSize);

	Protocol::S2S_RELAY_TO_WORLD_NOT relay;
	relay.set_accountid(accountId);
	relay.set_payload(innerBuffer->GetReadPtr(), innerBuffer->GetUseSize());

	pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));

	return true;
}
