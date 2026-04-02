#include "SCharacterCreatePanel.h"
#include "AuthWidgetStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

static const FSlateBrush& GetCardFillBrush()
{
	static FSlateBrush Brush;
	static bool bInit = false;
	if (!bInit)
	{
		bInit = true;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(12.0f, 12.0f, 12.0f, 12.0f);
		Brush.TintColor = FSlateColor(FLinearColor(0.03f, 0.03f, 0.05f, 0.82f));
	}
	return Brush;
}

static const FSlateBrush& GetCardBorderBrush()
{
	static FSlateBrush Brush;
	static bool bInit = false;
	if (!bInit)
	{
		bInit = true;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(12.0f, 12.0f, 12.0f, 12.0f);
		Brush.OutlineSettings.Width = 1.0f;
		Brush.OutlineSettings.Color = FLinearColor(0.75f, 0.80f, 0.95f, 0.30f);
		Brush.OutlineSettings.bUseBrushTransparency = true;
		Brush.TintColor = FSlateColor(FLinearColor::Transparent);
	}
	return Brush;
}

void SCharacterCreatePanel::Construct(const FArguments& InArgs)
{
	OnCreateRequested = InArgs._OnCreateRequested;

	static const FButtonStyle PrimaryStyle = AuthStyle::PrimaryButtonStyle();

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(540.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(&GetCardFillBrush())
			]

			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(&GetCardBorderBrush())
			]

			+ SOverlay::Slot()
			.Padding(FMargin(40.0f, 44.0f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("캐릭터 생성")))
					.Font(AuthStyle::TitleFont())
					.ColorAndOpacity(FLinearColor::White)
					.Justification(ETextJustify::Center)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 28.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("모험을 시작할 캐릭터의 이름을 입력하세요.")))
					.Font(AuthStyle::SmallFont())
					.ColorAndOpacity(FLinearColor(0.78f, 0.82f, 0.90f, 1.0f))
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("닉네임")))
					.Font(AuthStyle::BodyFont())
					.ColorAndOpacity(FLinearColor(0.90f, 0.92f, 0.96f, 1.0f))
					.Justification(ETextJustify::Center)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 20.0f)
				[
					AuthStyle::MakeInput(
						NameInputBox,
						FText::FromString(TEXT("2~16자 닉네임 입력...")),
						false,
						[this](const FText& Text, ETextCommit::Type CommitType)
						{
							HandleNameCommitted(Text, CommitType);
						})
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 16.0f)
				[
					SNew(SBox)
					.HeightOverride(56.0f)
					[
						SAssignNew(CreateButton, SButton)
						.ButtonStyle(&PrimaryStyle)
						.ButtonColorAndOpacity(AuthStyle::C::Primary)
						.OnClicked(FOnClicked::CreateRaw(this, &SCharacterCreatePanel::HandleCreateClicked))
						.ContentPadding(FMargin(0.0f, 10.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("생성하기")))
							.Font(AuthStyle::ButtonFont())
							.ColorAndOpacity(FLinearColor::White)
							.Justification(ETextJustify::Center)
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(StatusText, STextBlock)
					.Font(AuthStyle::SmallFont())
					.ColorAndOpacity(AuthStyle::C::Error)
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
				]
			]
		]
	];
}

void SCharacterCreatePanel::SetStatusMessage(const FString& Message, const bool bIsError)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Message));
		StatusText->SetColorAndOpacity(bIsError ? AuthStyle::C::Error : AuthStyle::C::Success);
	}
}

void SCharacterCreatePanel::SetCreateEnabled(const bool bEnabled)
{
	if (CreateButton.IsValid())
	{
		CreateButton->SetEnabled(bEnabled);
	}
}

void SCharacterCreatePanel::ClearInput()
{
	if (NameInputBox.IsValid())
	{
		NameInputBox->SetText(FText::GetEmpty());
	}
	SetStatusMessage(TEXT(""));
}

FReply SCharacterCreatePanel::HandleCreateClicked()
{
	if (!NameInputBox.IsValid())
	{
		return FReply::Handled();
	}

	const FString Name = NameInputBox->GetText().ToString().TrimStartAndEnd();
	if (Name.Len() < 2)
	{
		SetStatusMessage(TEXT("닉네임은 최소 2자 이상이어야 합니다."));
		return FReply::Handled();
	}
	if (Name.Len() > 16)
	{
		SetStatusMessage(TEXT("닉네임은 16자 이하여야 합니다."));
		return FReply::Handled();
	}

	SetStatusMessage(TEXT(""));
	SetCreateEnabled(false);
	OnCreateRequested.ExecuteIfBound(Name);
	return FReply::Handled();
}

void SCharacterCreatePanel::HandleNameCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		HandleCreateClicked();
	}
}
