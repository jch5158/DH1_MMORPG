#include "Network/CppNetEngine/NetSession.h"

#include "Async/Async.h"
#include "DH1_Client.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Network/PacketHandler/PacketServiceTypeHandler.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "UI/AuthWidgetStyle.h"

NetSession::NetSession(const int32 receiveBufferSize, const int32 maxPacketSize, AuthData ArgAuthData)
	:PacketSession(receiveBufferSize, maxPacketSize)
	, mAuthData(ArgAuthData)
{}

NetSession::~NetSession()
{
}

void NetSession::OnConnected()
{
	Protocol::C2S_LOGIN_REQ packet;
	packet.set_accountid(FCString::Atoi(*mAuthData.AccountId));
	packet.set_ticket(TCHAR_TO_UTF8(*mAuthData.Ticket));

	const auto SendBuffer = LoginPacketHandler::MakeSendBuffer(packet);
	Send(SendBuffer);
}

void NetSession::OnDisconnecting(const eDisconnectReason reason)
{
	mbDisconnectReason = reason;
}

void NetSession::OnDisconnected()
{
	const eDisconnectReason reason = mbDisconnectReason;

	if (reason == eDisconnectReason::Closed)
	{
		return;
	}

	const FText Message = (reason == eDisconnectReason::DuplicateLogin)
		? NSLOCTEXT("DH1", "DuplicateLoginKick", "다른 기기에서 로그인되어 연결이 종료되었습니다.")
		: NSLOCTEXT("DH1", "DisconnectedGeneric", "서버와의 연결이 끊어졌습니다.");

	AsyncTask(ENamedThreads::GameThread, [Message]()
	{
		if (GEngine == nullptr || GEngine->GameViewport == nullptr)
		{
			return;
		}

		UClientNetSubsystem::ForEachPlayClientNetSubsystem([](UClientNetSubsystem* Net)
		{
			Net->ClearNetworkSpawnedEntities();
		});

		TSharedRef<SWidget> Overlay = SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.Padding(FMargin(0))
				.BorderImage(AuthStyle::FlatBrush())
				.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.04f, 0.88f))
				[
					SNew(SBox)
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(Message)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
					.ColorAndOpacity(FLinearColor(1.f, 0.4f, 0.3f, 1.f))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("DH1", "RedirectToLogin", "잠시 후 로그인 화면으로 이동합니다..."))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
					.ColorAndOpacity(FLinearColor(0.6f, 0.65f, 0.7f, 1.f))
					.Justification(ETextJustify::Center)
				]
			];

		GEngine->GameViewport->AddViewportWidgetContent(Overlay, 2147483000);

		UE_LOG(LogDH1_Client, Warning, TEXT("Disconnected: %s"), *Message.ToString());

		if (UWorld* World = GEngine->GetCurrentPlayWorld())
		{
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([]()
			{
				if (GEngine == nullptr)
				{
					return;
				}
				for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
				{
					if (Ctx.WorldType == EWorldType::Game || Ctx.WorldType == EWorldType::PIE)
					{
						UGameplayStatics::OpenLevel(Ctx.World(), FName(TEXT("/Game/Levels/L_Login")));
						break;
					}
				}
			}), 3.0f, false);
		}
	});
}

void NetSession::OnSend(const int32 len)
{
}

void NetSession::OnReceivePacket(const byte* pBuffer, const int32 len)
{
	PacketSessionRef pSession = GetPacketSessionRef();

	if (pBuffer == nullptr || len < static_cast<int32>(sizeof(PacketHeader)))
	{
		pSession->Disconnect(eDisconnectReason::Kicked);
		return;
	}

	const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(pBuffer);
	if (static_cast<int32>(hdr->size) != len)
	{
		UE_LOG(LogDH1_Client, Error, TEXT("NetSession: 프레이밍 오류 — header.size(%u) != 수신 len(%d), 연결 종료"), hdr->size, len);
		pSession->Disconnect(eDisconnectReason::Kicked);
		return;
	}

	if (PacketServiceTypeHandler::HandlePacketServiceType(static_cast<uint16>(len), pBuffer, pSession) == false)
	{
		pSession->Disconnect(eDisconnectReason::Kicked);
	}
}
