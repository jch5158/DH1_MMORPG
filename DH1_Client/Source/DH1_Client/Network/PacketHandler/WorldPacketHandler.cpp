#include "WorldPacketHandler.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

bool WorldPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool WorldPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	return true;
}

bool WorldPacketHandler::HANDLE_S2C_WORLD_LIST_RES(const Protocol::S2C_WORLD_LIST_RES& packet, const PacketSessionRef& pSession)
{
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

		TArray<FWorldServerInfo> WorldList;
		for (int32 i = 0; i < packet.worlds_size(); ++i)
		{
			const auto& world = packet.worlds(i);
			FWorldServerInfo Info;
			Info.WorldId = world.worldid();
			Info.WorldName = UTF8_TO_TCHAR(world.worldname().c_str());
			Info.CurrentPlayers = world.currentplayers();
			Info.MaxPlayers = world.maxplayers();
			Info.Status = world.status();
			WorldList.Add(Info);
		}

		NetSubsystem->NotifyWorldList(WorldList);
		break;
	}

	return true;
}

bool WorldPacketHandler::HANDLE_S2C_WORLD_SELECT_RES(const Protocol::S2C_WORLD_SELECT_RES& packet, const PacketSessionRef& pSession)
{
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

		NetSubsystem->NotifyWorldSelectResult(static_cast<int32>(packet.result()));
		break;
	}

	return true;
}
