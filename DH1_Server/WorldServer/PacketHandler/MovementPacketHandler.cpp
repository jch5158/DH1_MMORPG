#include "pch.h"
#include "MovementPacketHandler.h"
#include "GameTickProcessor.h"
#include "GridManager.h"
#include "PlayerObject.h"
#include "WorldService.h"
#include "RelayContext.h"
#include "PacketHandler/GameSessionPacketHandler.h"
#include "GameSessionManager.h"

namespace
{
	template<typename T>
	NetSendBufferRef MakeMovementSendBuffer(const T& packet, const uint16 packetId)
	{
		const uint16 dataSize = static_cast<uint16>(packet.ByteSizeLong());
		const uint16 packetSize = dataSize + static_cast<uint16>(sizeof(PacketHeader));

		auto sendBuffer = cpp_net_engine::MakeSendBuffer(packetSize);
		byte* pBuffer = sendBuffer->Reserve(packetSize);
		if (pBuffer == nullptr)
		{
			return nullptr;
		}

		auto* header = reinterpret_cast<PacketHeader*>(pBuffer);
		header->size = packetSize;
		header->id = (static_cast<uint32>(Protocol::eServiceType::SERVICE_TYPE_MOVEMENT) << 16) | static_cast<uint32>(packetId);
		packet.SerializeToArray(&header[1], dataSize);
		sendBuffer->Commit(packetSize);

		return sendBuffer;
	}
}

bool MovementPacketHandler::Validate(const PacketSessionRef& pSession)
{
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
	const uint64 accountId = RelayContext::GetAccountId();
	if (accountId == 0)
	{
		NET_ENGINE_LOG_ERROR("MovementPacketHandler - SPAWN_POSITION_REQ, RelayContext accountId is 0");
		return false;
	}

	auto& worldService = ISingleton<WorldService>::GetInstance();
	const auto pGridManager = worldService.GetGridManagerRef();
	if (pGridManager == nullptr)
	{
		return false;
	}

	const auto pObject = pGridManager->GetObjectByAccountId(accountId);
	if (pObject == nullptr)
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler - SPAWN_POSITION_REQ, PlayerObject not found, accountId: {}", accountId);
		return false;
	}

	const auto pPlayer = std::static_pointer_cast<PlayerObject>(pObject);
	const float posX = pPlayer->GetPositionX();
	const float posY = pPlayer->GetPositionY();
	const float posZ = pPlayer->GetPositionZ();
	const float rotYaw = pPlayer->GetRotationYaw();

	// S2C_SPAWN_POSITION_RES → RELAY_TO_CLIENT_NOT
	Protocol::S2C_SPAWN_POSITION_RES response;
	response.mutable_position()->set_x(posX);
	response.mutable_position()->set_y(posY);
	response.mutable_position()->set_z(posZ);
	response.set_rotationyaw(rotYaw);
	pPlayer->WriteCharacterSheetTo(response);

	auto innerBuffer = MakeMovementSendBuffer(response, packet_id::S2C_SPAWN_POSITION_RES);

	Protocol::S2S_RELAY_TO_CLIENT_NOT relayPacket;
	relayPacket.set_gatewaysessionid(pPlayer->GetGatewaySessionId());
	relayPacket.set_payload(innerBuffer->GetReadPtr(), innerBuffer->GetUseSize());

	pSession->Send(GameSessionPacketHandler::MakeSendBuffer(relayPacket));

	NET_ENGINE_LOG_INFO("MovementPacketHandler - SPAWN_POSITION_RES sent, accountId: {}, displayNameLen: {}, pos: ({}, {}, {}), yaw: {}",
		accountId, pPlayer->GetDisplayName().size(), posX, posY, posZ, rotYaw);

	return true;
}

bool MovementPacketHandler::HANDLE_C2S_MOVE_TO_POSITION_REQ(const Protocol::C2S_MOVE_TO_POSITION_REQ& packet, const PacketSessionRef& pSession)
{
	const uint64 accountId = RelayContext::GetAccountId();
	if (accountId == 0)
	{
		NET_ENGINE_LOG_ERROR("MovementPacketHandler - MOVE_TO_POSITION_REQ, RelayContext accountId is 0");
		return false;
	}

	// 목적지 NaN/Inf 검증
	const float destX = packet.destination().x();
	const float destY = packet.destination().y();
	const float destZ = packet.destination().z();

	if (std::isnan(destX) || std::isnan(destY) || std::isnan(destZ) ||
		std::isinf(destX) || std::isinf(destY) || std::isinf(destZ))
	{
		NET_ENGINE_LOG_WARN("MovementPacketHandler (World) - Invalid destination, accountId: {}", accountId);
		return true;
	}

	const auto pGameTickProcessor = ISingleton<WorldService>::GetInstance().GetGameTickProcessorRef();
	if (pGameTickProcessor == nullptr)
	{
		return false;
	}

	MoveToPositionEntry entry;
	entry.mAccountId = accountId;
	entry.mSequenceId = packet.sequenceid();
	entry.mDestinationX = destX;
	entry.mDestinationY = destY;
	entry.mDestinationZ = destZ;
	entry.mClientTimestamp = packet.clienttimestamp();
	entry.mHasCurrentPosition = packet.has_currentposition();
	entry.mCurrentX = packet.currentposition().x();
	entry.mCurrentY = packet.currentposition().y();
	entry.mCurrentZ = packet.currentposition().z();

	pGameTickProcessor->EnqueueMoveToPosition(entry);

	NET_ENGINE_LOG_TRACE("MovementPacketHandler - MOVE_TO_POSITION_REQ received, accountId: {}, dest: ({:.1f}, {:.1f}, {:.1f})",
		accountId, destX, destY, destZ);

	return true;
}
