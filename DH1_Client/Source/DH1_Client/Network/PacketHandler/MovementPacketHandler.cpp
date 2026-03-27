#include "MovementPacketHandler.h"
#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogMovement, Log, All);

// 스폰 위치 대기용 static 변수
static FVector PendingSpawnPosition = FVector::ZeroVector;
static float PendingSpawnYaw = 0.0f;
static bool bHasPendingSpawn = false;

bool MovementPacketHandler_ConsumePendingSpawn(FVector& OutPos, float& OutYaw)
{
	if (!bHasPendingSpawn) return false;
	OutPos = PendingSpawnPosition;
	OutYaw = PendingSpawnYaw;
	bHasPendingSpawn = false;
	return true;
}

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
	const float posX = packet.position().x();
	const float posY = packet.position().y();
	const float posZ = packet.position().z();
	const float rotYaw = packet.rotationyaw();

	UE_LOG(LogMovement, Log, TEXT("MOVE_RESULT - seq: %u, pos: (%.1f, %.1f, %.1f), yaw: %.1f"),
		packet.sequenceid(), posX, posY, posZ, rotYaw);

	// TODO: Client-Side Prediction reconciliation

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

	// 위치를 static에 저장 — 캐릭터 BeginPlay에서 가져감
	PendingSpawnPosition = FVector(posX, posY, posZ);
	PendingSpawnYaw = rotYaw;
	bHasPendingSpawn = true;

	return true;
}
