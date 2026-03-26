#include "RealmPacketHandler.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

bool RealmPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

bool RealmPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	return true;
}

bool RealmPacketHandler::HANDLE_S2C_REALM_LIST_RES(const Protocol::S2C_REALM_LIST_RES& packet, const PacketSessionRef& pSession)
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

		TArray<FRealmServerInfo> RealmList;
		for (int32 i = 0; i < packet.realms_size(); ++i)
		{
			const auto& realm = packet.realms(i);
			FRealmServerInfo Info;
			Info.RealmId = realm.realmid();
			Info.RealmName = FString(UTF8_TO_TCHAR(realm.realmname().c_str()));
			Info.CurrentPlayers = realm.currentplayers();
			Info.MaxPlayers = realm.maxplayers();
			Info.Status = realm.status();
			RealmList.Add(Info);
		}

		NetSubsystem->NotifyRealmList(RealmList);
		break;
	}

	return true;
}

bool RealmPacketHandler::HANDLE_S2C_REALM_SELECT_RES(const Protocol::S2C_REALM_SELECT_RES& packet, const PacketSessionRef& pSession)
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

		NetSubsystem->NotifyRealmSelectResult(static_cast<int32>(packet.result()));
		break;
	}

	return true;
}
