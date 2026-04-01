#include "ChatPacketHandler.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Network/Dh1StringConv.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

bool ChatPacketHandler::Validate(const PacketSessionRef& pSession)
{
	(void)pSession;
	return true;
}

bool ChatPacketHandler::HANDLE_PACKET_ID_INVALID(const uint16 size, const uint16 packetId, const byte* pBuffer, const PacketSessionRef& pSession)
{
	(void)size;
	(void)packetId;
	(void)pBuffer;
	(void)pSession;
	return true;
}

bool ChatPacketHandler::HANDLE_S2C_CHAT_NOT(const Protocol::S2C_CHAT_NOT& packet, const PacketSessionRef& pSession)
{
	(void)pSession;

	if (GEngine == nullptr)
	{
		return true;
	}

	const int32 Ch = static_cast<int32>(packet.channel());
	const uint64 SenderId = packet.sender_account_id();
	const std::string NameUtf8 = packet.sender_display_name();
	const std::string MsgUtf8 = packet.message();
	const int64 Ts = packet.server_timestamp_ms();

	AsyncTask(ENamedThreads::GameThread, [Ch, SenderId, NameUtf8, MsgUtf8, Ts]()
		{
			const FString DisplayName = Dh1Utf8StdStringToFString(NameUtf8);
			const FString Message = Dh1Utf8StdStringToFString(MsgUtf8);

			UClientNetSubsystem::ForEachPlayClientNetSubsystem(
				[Ch, SenderId, DisplayName, Message, Ts](UClientNetSubsystem* NetSubsystem)
				{
					NetSubsystem->NotifyChatMessageReceived(Ch, SenderId, DisplayName, Message, Ts);
				});
		});

	return true;
}
