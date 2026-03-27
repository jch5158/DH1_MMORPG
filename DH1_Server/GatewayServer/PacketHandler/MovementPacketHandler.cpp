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
	const auto pClientSession = std::static_pointer_cast<ClientSession>(pSession);
	const uint64 accountId = pClientSession->GetAccountId();

	// 방향 벡터 NaN/Inf 검증
	const float dirX = packet.direction().x();
	const float dirY = packet.direction().y();
	const float dirZ = packet.direction().z();
	const float rotYaw = packet.rotationyaw();
	const float posZ = packet.positionz();

	if (std::isnan(dirX) || std::isnan(dirY) || std::isnan(dirZ) ||
		std::isinf(dirX) || std::isinf(dirY) || std::isinf(dirZ) ||
		std::isnan(rotYaw) || std::isinf(rotYaw) ||
		std::isnan(posZ) || std::isinf(posZ))
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler - Invalid float values, accountId: {}", accountId);
		return false;
	}

	// 방향 벡터 크기 검증 (정규화된 벡터여야 함, 약간의 오차 허용)
	const float dirLengthSq = dirX * dirX + dirY * dirY + dirZ * dirZ;
	if (dirLengthSq > 1.5f) // 정규화 + 부동소수점 오차 허용
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler - Direction vector too large ({:.2f}), accountId: {}", dirLengthSq, accountId);
		return false;
	}

	// 시퀀스 ID 검증 (단조 증가)
	const uint32 seq = packet.sequenceid();
	const uint32 lastSeq = pClientSession->GetLastMoveSequenceId();
	if (seq != 0 && seq <= lastSeq)
	{
		NET_ENGINE_LOG_TRACE("MovementPacketHandler - Stale sequenceId: {} <= {}, accountId: {}", seq, lastSeq, accountId);
		return true; // 무시 (연결 끊지 않음)
	}
	pClientSession->SetLastMoveSequenceId(seq);

	// 이동 패킷 속도 제한 (초당 25회 이상이면 무시)
	const int64 nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
	const int64 lastMoveMs = pClientSession->GetLastMoveTimestampMs();
	if (lastMoveMs > 0 && (nowMs - lastMoveMs) < 40) // 40ms = 25Hz
	{
		return true; // 속도 제한 - 무시
	}
	pClientSession->SetLastMoveTimestampMs(nowMs);

	auto& gatewayService = ISingleton<GatewayService>::GetInstance();
	const auto pWorldClientService = gatewayService.GetWorldClientServiceRef();
	if (pWorldClientService == nullptr)
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler - WorldClientService is null, dropping move packet, accountId: {}", accountId);
		return true;
	}

	const auto pWorldSession = pWorldClientService->GetFirstSessionRef();
	if (pWorldSession == nullptr)
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler - WorldSession is null, dropping move packet, accountId: {}", accountId);
		return true;
	}

	// 패킷을 재직렬화하여 relay payload로 전송
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
		Protocol::eServiceType::SERVICE_TYPE_MOVEMENT, packet_id::C2S_MOVE_INPUT_NOT);
	packet.SerializeToArray(&header[1], dataSize);
	innerBuffer->Commit(packetSize);

	Protocol::S2S_RELAY_TO_WORLD_NOT relay;
	relay.set_accountid(accountId);
	relay.set_payload(innerBuffer->GetReadPtr(), innerBuffer->GetUseSize());

	pWorldSession->Send(GameSessionPacketHandler::MakeSendBuffer(relay));

	NET_ENGINE_LOG_TRACE("MovementPacketHandler - MOVE_INPUT relayed, accountId: {}, seq: {}, dir: ({:.1f}, {:.1f}, {:.1f})",
		accountId, packet.sequenceid(), packet.direction().x(), packet.direction().y(), packet.direction().z());

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
