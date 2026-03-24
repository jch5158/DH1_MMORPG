#include "LoginPacketHandler.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

bool LoginPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer,
	const PacketSessionRef& pSession)
{
	return true;
}

bool LoginPacketHandler::HANDLE_S2C_LOGIN_RES(const Protocol::S2C_LOGIN_RES& packet, const PacketSessionRef& pSession)
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

		NetSubsystem->NotifyLoginResult(static_cast<int32>(packet.result()));
		break;
	}

	return true;
}
