#include "RealmSelectWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

void SRealmSelectWidget::Construct(const FArguments& InArgs)
{
	RealmList = InArgs._RealmList;

	// 카드 스타일 버튼 브러시 (더 어두운 테마)
	static FButtonStyle CardButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.04f, 0.06f, 0.95f), 6.0f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.08f, 0.08f, 0.14f, 1.0f), 6.0f))
		.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.03f, 0.05f, 1.0f), 6.0f))
		.SetNormalPadding(FMargin(0))
		.SetPressedPadding(FMargin(0));

	TSharedRef<SVerticalBox> ListBox = SNew(SVerticalBox);

	for (const auto& Realm : RealmList)
	{
		FString StatusStr;
		FLinearColor StatusColor;
		FLinearColor StatusDotColor;
		switch (Realm.Status)
		{
		case 0:
			StatusStr = TEXT("Online");
			StatusColor = FLinearColor(0.2f, 0.9f, 0.4f);
			StatusDotColor = FLinearColor(0.2f, 0.9f, 0.4f);
			break;
		case 1:
			StatusStr = TEXT("Maintenance");
			StatusColor = FLinearColor(0.95f, 0.8f, 0.2f);
			StatusDotColor = FLinearColor(0.95f, 0.8f, 0.2f);
			break;
		case 2:
			StatusStr = TEXT("Full");
			StatusColor = FLinearColor(0.95f, 0.3f, 0.3f);
			StatusDotColor = FLinearColor(0.95f, 0.3f, 0.3f);
			break;
		default:
			StatusStr = TEXT("Unknown");
			StatusColor = FLinearColor(0.5f, 0.5f, 0.5f);
			StatusDotColor = FLinearColor(0.5f, 0.5f, 0.5f);
			break;
		}

		// 접속률 계산
		const float PlayerRatio = Realm.MaxPlayers > 0
			? static_cast<float>(Realm.CurrentPlayers) / static_cast<float>(Realm.MaxPlayers)
			: 0.0f;

		FLinearColor BarColor;
		if (PlayerRatio < 0.5f)
		{
			BarColor = FLinearColor(0.2f, 0.7f, 0.4f);
		}
		else if (PlayerRatio < 0.8f)
		{
			BarColor = FLinearColor(0.9f, 0.7f, 0.2f);
		}
		else
		{
			BarColor = FLinearColor(0.9f, 0.3f, 0.3f);
		}

		const int32 CapturedRealmId = Realm.RealmId;

		ListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(SButton)
			.ButtonStyle(&CardButtonStyle)
			.OnClicked_Lambda([this, CapturedRealmId]() -> FReply
			{
				OnRealmClicked(CapturedRealmId);
				return FReply::Handled();
			})
			.ContentPadding(FMargin(24.0f, 16.0f))
			[
				SNew(SVerticalBox)
				// 상단: 렐름 이름 + 상태
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					// 렐름 이름
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Realm.RealmName))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
						.ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.92f))
					]
					// 상태 뱃지
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SBorder)
						.BorderImage(new FSlateRoundedBoxBrush(StatusDotColor.CopyWithNewOpacity(0.15f), 4.0f))
						.Padding(FMargin(10.0f, 4.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(StatusStr))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							.ColorAndOpacity(StatusColor)
						]
					]
				]
				// 하단: 접속자 바
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Players")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
							.ColorAndOpacity(FLinearColor(0.35f, 0.35f, 0.42f))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%d / %d"), Realm.CurrentPlayers, Realm.MaxPlayers)))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
							.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.6f))
						]
					]
					// 프로그레스 바 (배경)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.HeightOverride(4.0f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SBorder)
								.BorderImage(new FSlateRoundedBoxBrush(FLinearColor(0.08f, 0.08f, 0.12f), 2.0f))
							]
							+ SOverlay::Slot()
							.HAlign(HAlign_Left)
							[
								SNew(SBox)
								.WidthOverride(500.0f * PlayerRatio)
								[
									SNew(SBorder)
									.BorderImage(new FSlateRoundedBoxBrush(BarColor, 2.0f))
								]
							]
						]
					]
				]
			]
		];
	}

	ChildSlot
	[
		// 풀스크린 어두운 배경
		SNew(SBorder)
		.BorderImage(new FSlateColorBrush(FLinearColor(0.01f, 0.01f, 0.02f, 0.96f)))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			// 메인 패널
			SNew(SBorder)
			.BorderImage(new FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.03f, 0.05f, 0.98f), 10.0f))
			.Padding(FMargin(48.0f, 36.0f))
			[
				SNew(SVerticalBox)
				// 타이틀
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Select Realm")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
					.ColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.95f))
				]
				// 부제목
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 24.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Choose a realm to play")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
					.ColorAndOpacity(FLinearColor(0.35f, 0.35f, 0.45f))
				]
				// 구분선
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 16.0f)
				[
					SNew(SBox)
					.HeightOverride(1.0f)
					[
						SNew(SBorder)
						.BorderImage(new FSlateColorBrush(FLinearColor(0.08f, 0.08f, 0.12f)))
					]
				]
				// 렐름 리스트 (여러 렐름 스크롤 대응)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.MinDesiredWidth(550.0f)
					.MaxDesiredHeight(600.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							ListBox
						]
					]
				]
			]
		]
	];
}

void SRealmSelectWidget::OnRealmClicked(const int32 RealmId)
{
	if (GEngine == nullptr)
	{
		return;
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

		NetSubsystem->RequestRealmSelect(RealmId);
		break;
	}
}
