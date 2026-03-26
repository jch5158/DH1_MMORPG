#include "pch.h"
#include "MovementPacketHandler.h"
#include "GameTickProcessor.h"
#include "MoveInputEntry.h"
#include "WorldService.h"
#include "RelayContext.h"

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
	const uint64 accountId = RelayContext::GetAccountId();
	if (accountId == 0)
	{
		NET_ENGINE_LOG_ERROR("MovementPacketHandler - RelayContext accountId is 0");
		return false;
	}

	const auto pGameTickProcessor = ISingleton<WorldService>::GetInstance().GetGameTickProcessorRef();
	if (pGameTickProcessor == nullptr)
	{
		return false;
	}

	MoveInputEntry entry;
	entry.mAccountId = accountId;
	entry.mSequenceId = packet.sequenceid();
	entry.mDirectionX = packet.direction().x();
	entry.mDirectionY = packet.direction().y();
	entry.mDirectionZ = packet.direction().z();
	entry.mRotationYaw = packet.rotationyaw();
	entry.mClientTimestamp = packet.clienttimestamp();

	pGameTickProcessor->EnqueueMoveInput(entry);

	NET_ENGINE_LOG_TRACE("MovementPacketHandler - MOVE_INPUT received, accountId: {}, seq: {}, dir: ({:.1f}, {:.1f}, {:.1f})",
		accountId, packet.sequenceid(), packet.direction().x(), packet.direction().y(), packet.direction().z());

	return true;
}
