#include "MovementPacketHandler.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Math/UnrealMathUtility.h"
#include "Network/Dh1StringConv.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

#include <string>

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

	if (packet.entities_size() == 0)
	{
		return true;
	}

	TArray<FNetworkEntitySpawnData> Entities;
	Entities.Reserve(packet.entities_size());
	for (int32 i = 0; i < packet.entities_size(); ++i)
	{
		const auto& e = packet.entities(i);
		FNetworkEntitySpawnData Data;
		Data.EntityId = e.entityid();
		Data.Position = FVector(e.position().x(), e.position().y(), e.position().z());
		Data.YawDegrees = e.rotationyaw();
		Entities.Add(MoveTemp(Data));
	}

	AsyncTask(ENamedThreads::GameThread, [Entities = MoveTemp(Entities)]()
		{
			if (GEngine == nullptr) { return; }
			UClientNetSubsystem::ForEachPlayClientNetSubsystem(
				[&Entities](UClientNetSubsystem* Net)
				{
					Net->ApplyNetworkEntitiesEntered(Entities);
				});
		});

	return true;
}

bool MovementPacketHandler::HANDLE_S2C_ENTITY_ENTER_NOT(const Protocol::S2C_ENTITY_ENTER_NOT& packet, const PacketSessionRef& pSession)
{
	UE_LOG(LogMovement, Log, TEXT("ENTITY_ENTER - entityCount: %d"), packet.entities_size());

	if (packet.entities_size() == 0)
	{
		return true;
	}

	TArray<FNetworkEntitySpawnData> Entities;
	Entities.Reserve(packet.entities_size());
	for (int32 i = 0; i < packet.entities_size(); ++i)
	{
		const auto& e = packet.entities(i);
		FNetworkEntitySpawnData Data;
		Data.EntityId = e.entityid();
		Data.Position = FVector(e.position().x(), e.position().y(), e.position().z());
		Data.YawDegrees = e.rotationyaw();
		if (!e.displayname().empty())
		{
			Data.DisplayName = UTF8_TO_TCHAR(e.displayname().c_str());
			Data.Level = e.level();
			Data.CurrentHP = e.currenthp();
			Data.MaxHP = e.maxhp();
		}
		Entities.Add(MoveTemp(Data));

		UE_LOG(LogMovement, Log, TEXT("  entity: %llu, pos: (%.1f, %.1f, %.1f), name: %s"),
			e.entityid(), e.position().x(), e.position().y(), e.position().z(),
			Data.DisplayName.IsEmpty() ? TEXT("(none)") : *Data.DisplayName);
	}

	AsyncTask(ENamedThreads::GameThread, [Entities = MoveTemp(Entities)]()
		{
			if (GEngine == nullptr) { return; }
			UClientNetSubsystem::ForEachPlayClientNetSubsystem(
				[&Entities](UClientNetSubsystem* Net)
				{
					Net->ApplyNetworkEntitiesEntered(Entities);
				});
		});

	return true;
}

bool MovementPacketHandler::HANDLE_S2C_ENTITY_LEAVE_NOT(const Protocol::S2C_ENTITY_LEAVE_NOT& packet, const PacketSessionRef& pSession)
{
	UE_LOG(LogMovement, Log, TEXT("ENTITY_LEAVE - entityCount: %d"), packet.entityids_size());

	if (packet.entityids_size() == 0)
	{
		return true;
	}

	TArray<uint64> EntityIds;
	EntityIds.Reserve(packet.entityids_size());
	for (int32 i = 0; i < packet.entityids_size(); ++i)
	{
		EntityIds.Add(packet.entityids(i));
	}

	AsyncTask(ENamedThreads::GameThread, [EntityIds = MoveTemp(EntityIds)]()
		{
			if (GEngine == nullptr) { return; }
			UClientNetSubsystem::ForEachPlayClientNetSubsystem(
				[&EntityIds](UClientNetSubsystem* Net)
				{
					Net->ApplyNetworkEntitiesLeft(EntityIds);
				});
		});

	return true;
}

bool MovementPacketHandler::HANDLE_S2C_SPAWN_POSITION_RES(const Protocol::S2C_SPAWN_POSITION_RES& packet, const PacketSessionRef& pSession)
{
	float posX = 0.f;
	float posY = 0.f;
	float posZ = 0.f;
	if (packet.has_position())
	{
		const auto& p = packet.position();
		posX = p.x();
		posY = p.y();
		posZ = p.z();
	}

	float rotYaw = packet.rotationyaw();
	if (!FMath::IsFinite(posX) || !FMath::IsFinite(posY) || !FMath::IsFinite(posZ) || !FMath::IsFinite(rotYaw))
	{
		UE_LOG(LogMovement, Error, TEXT("SPAWN_POSITION_RES: non-finite pos or yaw; using (0,0,0) yaw 0"));
		posX = posY = posZ = 0.f;
		rotYaw = 0.f;
	}

	const std::string displayNameUtf8 = packet.displayname();

	int32 Level = static_cast<int32>(packet.level());
	if (Level < 1)
	{
		Level = 1;
	}
	Level = FMath::Clamp(Level, 1, 9999);

	float MaxHP = 100.f;
	if (FMath::IsFinite(packet.maxhp()) && packet.maxhp() > 0.f)
	{
		MaxHP = packet.maxhp();
	}
	float CurHP = FMath::IsFinite(packet.currenthp()) ? packet.currenthp() : MaxHP;
	CurHP = FMath::Clamp(CurHP, 0.f, MaxHP);

	const int32 RawNameBytes = static_cast<int32>(displayNameUtf8.size());
	const FVector Pos(posX, posY, posZ);

	AsyncTask(ENamedThreads::GameThread,
		[Pos, rotYaw, displayNameUtf8, Level, CurHP, MaxHP, RawNameBytes]()
		{
			FString DisplayName = Dh1Utf8StdStringToFString(displayNameUtf8);
			DisplayName.TrimStartAndEndInline();
			if (DisplayName.IsEmpty())
			{
				DisplayName = TEXT("Adventurer");
			}

			UE_LOG(LogMovement, Log, TEXT("SPAWN_POSITION_RES pos=(%.1f,%.1f,%.1f) yaw=%.1f rawNameBytes=%d name=%s Lv=%d HP=%.1f/%.1f"),
				Pos.X, Pos.Y, Pos.Z, rotYaw, RawNameBytes, *DisplayName, Level, CurHP, MaxHP);

			if (GEngine == nullptr)
			{
				return;
			}

			UClientNetSubsystem::ForEachPlayClientNetSubsystem(
				[Pos, rotYaw, DisplayName, Level, CurHP, MaxHP](UClientNetSubsystem* NetSubsystem)
				{
					NetSubsystem->NotifySpawnPositionWithCharacterSheet(Pos, rotYaw, DisplayName, Level, CurHP, MaxHP);
				});
		});

	(void)pSession;
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

bool MovementPacketHandler::HANDLE_S2C_CHARACTER_CREATE_NOT(const Protocol::S2C_CHARACTER_CREATE_NOT& packet, const PacketSessionRef& pSession)
{
	UE_LOG(LogMovement, Log, TEXT("CHARACTER_CREATE_NOT - character creation required"));

	AsyncTask(ENamedThreads::GameThread, []()
		{
			if (GEngine == nullptr) { return; }
			UClientNetSubsystem::ForEachPlayClientNetSubsystem(
				[](UClientNetSubsystem* Net)
				{
					Net->NotifyCharacterCreateRequired();
				});
		});

	return true;
}

bool MovementPacketHandler::HANDLE_S2C_CREATE_CHARACTER_RES(const Protocol::S2C_CREATE_CHARACTER_RES& packet, const PacketSessionRef& pSession)
{
	const int32 Result = static_cast<int32>(packet.result());
	const FString Message = UTF8_TO_TCHAR(packet.message().c_str());

	UE_LOG(LogMovement, Log, TEXT("CREATE_CHARACTER_RES - result: %d, msg: %s"), Result, *Message);

	AsyncTask(ENamedThreads::GameThread, [Result, Message]()
		{
			if (GEngine == nullptr) { return; }
			UClientNetSubsystem::ForEachPlayClientNetSubsystem(
				[Result, Message](UClientNetSubsystem* Net)
				{
					Net->NotifyCharacterCreateResult(Result, Message);
				});
		});

	return true;
}
