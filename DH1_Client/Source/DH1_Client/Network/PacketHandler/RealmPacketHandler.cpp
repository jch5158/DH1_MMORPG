#include "RealmPacketHandler.h"
#include "Engine/Engine.h"
#include "Network/Dh1StringConv.h"
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

	TArray<FRealmServerInfo> RealmList;
	for (int32 i = 0; i < packet.realms_size(); ++i)
	{
		const auto& realm = packet.realms(i);
		FRealmServerInfo Info;
		Info.RealmId = realm.realmid();
		Info.RealmName = Dh1Utf8StdStringToFString(realm.realmname());
		Info.CurrentPlayers = realm.currentplayers();
		Info.MaxPlayers = realm.maxplayers();
		Info.Status = realm.status();
		RealmList.Add(Info);
	}

	UClientNetSubsystem::ForEachPlayClientNetSubsystem(
		[RealmList](UClientNetSubsystem* NetSubsystem)
		{
			NetSubsystem->NotifyRealmList(RealmList);
		});

	return true;
}

bool RealmPacketHandler::HANDLE_S2C_REALM_SELECT_RES(const Protocol::S2C_REALM_SELECT_RES& packet, const PacketSessionRef& pSession)
{
	if (GEngine == nullptr)
	{
		return false;
	}

	const int32 SelectResult = static_cast<int32>(packet.result());
	UClientNetSubsystem::ForEachPlayClientNetSubsystem(
		[SelectResult](UClientNetSubsystem* NetSubsystem)
		{
			NetSubsystem->NotifyRealmSelectResult(SelectResult);
		});

	return true;
}
