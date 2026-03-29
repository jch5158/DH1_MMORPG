#include "RealmSelectWidget.h"
#include "AuthWidgetStyle.h"
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

	static FButtonStyle CardButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(AuthStyle::C::CardBg, 6.0f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.055f, 0.042f, 0.042f, 1.0f), 6.0f))
		.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.025f, 0.018f, 0.018f, 1.0f), 6.0f))
		.SetNormalPadding(FMargin(0))
		.SetPressedPadding(FMargin(0));

	TSharedRef<SVerticalBox> ListBox = SNew(SVerticalBox);

	for (const auto& Realm : RealmList)
	{
		FString StatusStr;
		FLinearColor StatusColor;
		switch (Realm.Status)
		{
		case 0:
			StatusStr = TEXT("Online");
			StatusColor = AuthStyle::C::Success;
			break;
		case 1:
			StatusStr = TEXT("점검 중");
			StatusColor = FLinearColor(0.95f, 0.75f, 0.15f, 1.0f);
			break;
		case 2:
			StatusStr = TEXT("포화");
			StatusColor = AuthStyle::C::Error;
			break;
		default:
			StatusStr = TEXT("Unknown");
			StatusColor = AuthStyle::C::Dim;
			break;
		}

		const float PlayerRatio = Realm.MaxPlayers > 0
			? static_cast<float>(Realm.CurrentPlayers) / static_cast<float>(Realm.MaxPlayers)
			: 0.0f;

		FLinearColor BarColor;
		if (PlayerRatio < 0.5f)
		{
			BarColor = AuthStyle::C::Success;
		}
		else if (PlayerRatio < 0.8f)
		{
			BarColor = FLinearColor(0.9f, 0.7f, 0.15f, 1.0f);
		}
		else
		{
			BarColor = AuthStyle::C::Error;
		}

		const int32 CapturedRealmId = Realm.RealmId;

		ListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			// 카드 외곽 테두리 (blood red 1px border)
			SNew(SBorder)
			.BorderImage(AuthStyle::FlatBrush())
			.BorderBackgroundColor(AuthStyle::C::CardBorder)
			.Padding(FMargin(1.0f))
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

					// 상단: 렐름 이름 + 상태 뱃지
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Realm.RealmName))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
							.ColorAndOpacity(AuthStyle::C::Title)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SBorder)
							.BorderImage(new FSlateRoundedBoxBrush(StatusColor.CopyWithNewOpacity(0.15f), 4.0f))
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
								.Text(FText::FromString(TEXT("접속자")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
								.ColorAndOpacity(AuthStyle::C::Dim)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.Text(FText::FromString(FString::Printf(TEXT("%d / %d"), Realm.CurrentPlayers, Realm.MaxPlayers)))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
								.ColorAndOpacity(AuthStyle::C::Body)
							]
						]

						// 프로그레스 바 배경
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
									.BorderImage(new FSlateRoundedBoxBrush(AuthStyle::C::InputBg, 2.0f))
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
			]
		];
	}

	// 메인 카드 위젯 생성
	TSharedRef<SWidget> MainCard =
		SNew(SBorder)
		.BorderImage(AuthStyle::FlatBrush())
		.BorderBackgroundColor(AuthStyle::C::CardBorder)
		.Padding(FMargin(1.0f))
		[
			SNew(SBorder)
			.BorderImage(AuthStyle::FlatBrush())
			.BorderBackgroundColor(AuthStyle::C::CardBg)
			.Padding(FMargin(48.0f, 36.0f))
			[
				SNew(SVerticalBox)

				// 타이틀
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("서버 선택")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
					.ColorAndOpacity(AuthStyle::C::Title)
				]

				// 부제목
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 20.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("접속할 서버를 선택하세요")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
					.ColorAndOpacity(AuthStyle::C::Dim)
				]

				// Crimson 구분선
				+ SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(1.0f)
				.Padding(0.0f, 0.0f, 0.0f, 20.0f)
				[
					SNew(SBorder)
					.BorderImage(AuthStyle::FlatBrush())
					.BorderBackgroundColor(AuthStyle::C::CrimsonDim)
				]

				// 렐름 리스트
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
		];

	ChildSlot
	[
		AuthStyle::MakeVampireBackground(MainCard)
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
