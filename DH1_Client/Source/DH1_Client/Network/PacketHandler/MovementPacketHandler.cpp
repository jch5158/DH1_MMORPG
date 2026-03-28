#include "MovementPacketHandler.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMovement, Log, All);

bool MovementPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool MovementPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	return true;
}

bool MovementPacketHandler::HANDLE_S2C_MOVE_RESULT_NOT(const Protocol::S2C_MOVE_RESULT_NOT& packet, const PacketSessionRef& pSession)
{
	return true;
}

bool MovementPacketHandler::HANDLE_S2C_ENTITY_SNAPSHOT_NOT(const Protocol::S2C_ENTITY_SNAPSHOT_NOT& packet, const PacketSessionRef& pSession)
{
	UE_LOG(LogMovement, Log, TEXT("ENTITY_SNAPSHOT - entityCount: %d, timestamp: %lld"),
		packet.entities_size(), packet.servertimestamp());

	// TODO: Entity interpolation update
	return true;
}

bool MovementPacketHandler::HANDLE_S2C_ENTITY_ENTER_NOT(const Protocol::S2C_ENTITY_ENTER_NOT& packet, const PacketSessionRef& pSession)
{
	UE_LOG(LogMovement, Log, TEXT("ENTITY_ENTER - entityCount: %d"), packet.entities_size());

	for (int32 i = 0; i < packet.entities_size(); ++i)
	{
		const auto& entity = packet.entities(i);
		UE_LOG(LogMovement, Log, TEXT("  entity: %llu, pos: (%.1f, %.1f, %.1f)"),
			entity.entityid(), entity.position().x(), entity.position().y(), entity.position().z());
	}

	// TODO: Spawn nearby entities
	return true;
}

bool MovementPacketHandler::HANDLE_S2C_ENTITY_LEAVE_NOT(const Protocol::S2C_ENTITY_LEAVE_NOT& packet, const PacketSessionRef& pSession)
{
	UE_LOG(LogMovement, Log, TEXT("ENTITY_LEAVE - entityCount: %d"), packet.entityids_size());

	// TODO: Despawn entities
	return true;
}

bool MovementPacketHandler::HANDLE_S2C_SPAWN_POSITION_RES(const Protocol::S2C_SPAWN_POSITION_RES& packet, const PacketSessionRef& pSession)
{
	const float posX = packet.position().x();
	const float posY = packet.position().y();
	const float posZ = packet.position().z();
	const float rotYaw = packet.rotationyaw();

	UE_LOG(LogMovement, Warning, TEXT("SPAWN_POSITION_RES - pos: (%.1f, %.1f, %.1f), yaw: %.1f"),
		posX, posY, posZ, rotYaw);

	if (GEngine == nullptr)
	{
		return false;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UGameInstance* GameInstance = Context.OwningGameInstance;
		if (GameInstance == nullptr)
		{
			continue;
		}

		UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>();
		if (NetSubsystem == nullptr)
		{
			continue;
		}

		NetSubsystem->NotifySpawnPosition(FVector(posX, posY, posZ), rotYaw);
		break;
	}

	return true;
}

bool MovementPacketHandler::HANDLE_S2C_MOVE_PATH_RES(const Protocol::S2C_MOVE_PATH_RES& packet, const PacketSessionRef& pSession)
{
	UE_LOG(LogMovement, Log, TEXT("MOVE_PATH_RES - seq: %u, waypoints: %d, speed: %.1f"),
		packet.sequenceid(), packet.waypoints_size(), packet.movespeed());

	if (GEngine == nullptr)
	{
		return false;
	}

	TArray<FVector> Waypoints;
	Waypoints.Reserve(packet.waypoints_size());
	for (int32 i = 0; i < packet.waypoints_size(); ++i)
	{
		const auto& wp = packet.waypoints(i);
		Waypoints.Add(FVector(wp.x(), wp.y(), wp.z()));
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UGameInstance* GameInstance = Context.OwningGameInstance;
		if (GameInstance == nullptr)
		{
			continue;
		}

		UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>();
		if (NetSubsystem == nullptr)
		{
			continue;
		}

		NetSubsystem->NotifyMovePath(packet.sequenceid(), Waypoints, packet.movespeed());
		break;
	}

	return true;
}

bool MovementPacketHandler::HANDLE_S2C_POSITION_CORRECTION_NOT(const Protocol::S2C_POSITION_CORRECTION_NOT& packet, const PacketSessionRef& pSession)
{
	const float posX = packet.correctedposition().x();
	const float posY = packet.correctedposition().y();
	const float posZ = packet.correctedposition().z();

	UE_LOG(LogMovement, Warning, TEXT("POSITION_CORRECTION - pos: (%.1f, %.1f, %.1f)"), posX, posY, posZ);

	if (GEngine == nullptr)
	{
		return false;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UGameInstance* GameInstance = Context.OwningGameInstance;
		if (GameInstance == nullptr)
		{
			continue;
		}

		UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>();
		if (NetSubsystem == nullptr)
		{
			continue;
		}

		NetSubsystem->NotifyPositionCorrection(FVector(posX, posY, posZ));
		break;
	}

	return true;
}
