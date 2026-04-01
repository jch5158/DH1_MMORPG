#include "LoginPacketHandler.h"
#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Network/Dh1StringConv.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "UI/AuthWidgetStyle.h"

bool LoginPacketHandler::Validate(const PacketSessionRef& pSession)
{
	return true;
}

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

bool LoginPacketHandler::HANDLE_S2C_KICK_NOT(const Protocol::S2C_KICK_NOT& packet, const PacketSessionRef& pSession)
{
	const std::string reasonUtf8 = packet.reason();

	AsyncTask(ENamedThreads::GameThread, [reasonUtf8]()
	{
		if (GEngine == nullptr || GEngine->GameViewport == nullptr)
		{
			return;
		}

		const FString Reason = reasonUtf8.empty()
			? TEXT("서버와의 연결이 끊어졌습니다.")
			: Dh1Utf8StdStringToFString(reasonUtf8);

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
					.Text(FText::FromString(Reason))
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

	pSession->Disconnect(eDisconnectReason::DuplicateLogin);
	return true;
}
