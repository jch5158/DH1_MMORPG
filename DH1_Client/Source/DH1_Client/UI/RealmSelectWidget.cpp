#include "RealmSelectWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

void SRealmSelectWidget::Construct(const FArguments& InArgs)
{
	RealmList = InArgs._RealmList;

	TSharedRef<SVerticalBox> ListBox = SNew(SVerticalBox);

	for (const auto& Realm : RealmList)
	{
		FString StatusStr;
		FLinearColor StatusColor;
		switch (Realm.Status)
		{
		case 0: StatusStr = TEXT("Online"); StatusColor = FLinearColor::Green; break;
		case 1: StatusStr = TEXT("Maintenance"); StatusColor = FLinearColor::Yellow; break;
		case 2: StatusStr = TEXT("Full"); StatusColor = FLinearColor::Red; break;
		default: StatusStr = TEXT("Unknown"); StatusColor = FLinearColor::Gray; break;
		}

		const int32 CapturedRealmId = Realm.RealmId;

		ListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(SButton)
			.OnClicked_Lambda([this, CapturedRealmId]() -> FReply
			{
				OnRealmClicked(CapturedRealmId);
				return FReply::Handled();
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(20.0f, 10.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Realm.RealmName))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 20.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%d / %d"), Realm.CurrentPlayers, Realm.MaxPlayers)))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 20.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(StatusStr))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.ColorAndOpacity(StatusColor)
				]
			]
		];
	}

	ChildSlot
	[
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 30.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Select Realm")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
				.ColorAndOpacity(FLinearColor::White)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.MinDesiredWidth(500.0f)
				[
					ListBox
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
