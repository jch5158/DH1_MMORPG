#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SBoxPanel.h"

namespace AuthStyle
{
	// =========================================================================
	// Color Palette  (Dark Gothic / Vampire theme)
	// =========================================================================
	namespace C
	{
		// Background — near-black with faint warm undertone
		inline const FLinearColor ScreenBg     = FLinearColor(0.012f, 0.010f, 0.010f, 0.99f);
		inline const FLinearColor CardBg       = FLinearColor(0.038f, 0.030f, 0.030f, 1.0f);
		inline const FLinearColor CardBorder   = FLinearColor(0.50f,  0.04f,  0.06f,  1.0f);  // blood red border

		// Crimson accent (vampire theme)
		inline const FLinearColor Crimson      = FLinearColor(0.65f,  0.04f,  0.06f,  1.0f);
		inline const FLinearColor CrimsonDim   = FLinearColor(0.65f,  0.04f,  0.06f,  0.40f); // separator

		// Input
		inline const FLinearColor InputBg      = FLinearColor(0.022f, 0.018f, 0.018f, 1.0f);
		inline const FLinearColor InputFg      = FLinearColor(0.90f,  0.88f,  0.85f,  1.0f);
		inline const FLinearColor InputHint    = FLinearColor(0.38f,  0.35f,  0.35f,  1.0f);

		// Primary button — blood red
		inline const FLinearColor Primary      = FLinearColor(0.45f, 0.04f, 0.06f, 1.0f);
		inline const FLinearColor PriHover     = FLinearColor(0.60f, 0.05f, 0.08f, 1.0f);
		inline const FLinearColor PriPress     = FLinearColor(0.30f, 0.02f, 0.04f, 1.0f);

		// Secondary button — dark charcoal
		inline const FLinearColor Secondary    = FLinearColor(0.10f, 0.09f, 0.09f, 1.0f);
		inline const FLinearColor SecHover     = FLinearColor(0.16f, 0.14f, 0.14f, 1.0f);
		inline const FLinearColor SecPress     = FLinearColor(0.06f, 0.05f, 0.05f, 1.0f);

		// Text
		inline const FLinearColor Title        = FLinearColor(0.90f, 0.88f, 0.86f, 1.0f);  // cool off-white
		inline const FLinearColor Body         = FLinearColor(0.75f, 0.73f, 0.72f, 1.0f);
		inline const FLinearColor Dim          = FLinearColor(0.45f, 0.43f, 0.42f, 1.0f);
		inline const FLinearColor Error        = FLinearColor(0.95f, 0.25f, 0.25f, 1.0f);
		inline const FLinearColor Success      = FLinearColor(0.25f, 0.85f, 0.40f, 1.0f);
		inline const FLinearColor BtnText      = FLinearColor(1.0f,  1.0f,  1.0f,  1.0f);
	}

	// =========================================================================
	// Fonts
	// =========================================================================
	inline FSlateFontInfo TitleFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", 32);
	}

	inline FSlateFontInfo BodyFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", 18);
	}

	inline FSlateFontInfo SmallFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", 16);
	}

	inline FSlateFontInfo ButtonFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", 19);
	}

	// =========================================================================
	// Brush helpers
	// =========================================================================
	inline const FSlateBrush* FlatBrush()
	{
		return FCoreStyle::Get().GetBrush("GenericWhiteBox");
	}

	// =========================================================================
	// Button style
	// =========================================================================
	inline FButtonStyle PrimaryButtonStyle()
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.Normal.TintColor = FSlateColor(C::Primary);
		Style.Hovered.TintColor = FSlateColor(C::PriHover);
		Style.Pressed.TintColor = FSlateColor(C::PriPress);
		return Style;
	}

	inline FButtonStyle SecondaryButtonStyle()
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.Normal.TintColor = FSlateColor(C::Secondary);
		Style.Hovered.TintColor = FSlateColor(C::SecHover);
		Style.Pressed.TintColor = FSlateColor(C::SecPress);
		return Style;
	}

	// =========================================================================
	// EditableTextBox style
	// =========================================================================
	inline FEditableTextBoxStyle InputStyle()
	{
		FEditableTextBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
		Style.BackgroundImageNormal.TintColor = FSlateColor(C::InputBg);
		Style.BackgroundImageHovered.TintColor = FSlateColor(FLinearColor(0.08f, 0.08f, 0.12f, 1.0f));
		Style.BackgroundImageFocused.TintColor = FSlateColor(FLinearColor(0.08f, 0.08f, 0.12f, 1.0f));
		Style.BackgroundImageReadOnly.TintColor = FSlateColor(C::InputBg);
		Style.ForegroundColor = FSlateColor(C::InputFg);
		Style.TextStyle.Font = BodyFont();
		Style.Padding = FMargin(12.0f, 8.0f);
		return Style;
	}

	// =========================================================================
	// Composite helpers
	// =========================================================================
	inline FSlateFontInfo InputFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", 18);
	}

	inline TSharedRef<SWidget> MakeInput(TSharedPtr<SEditableTextBox>& OutBox, const FText& Hint, bool bIsPassword = false,
		TFunction<void(const FText&, ETextCommit::Type)> OnCommitted = nullptr)
	{
		return SNew(SBox)
			.HeightOverride(48.0f)
			[
				SAssignNew(OutBox, SEditableTextBox)
				.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
				.HintText(Hint)
				.IsPassword(bIsPassword)
				.Font(InputFont())
				.BackgroundColor(C::InputBg)
				.ForegroundColor(C::InputFg)
				.OnTextCommitted_Lambda([OnCommitted](const FText& Text, ETextCommit::Type CommitType)
					{
						if (OnCommitted) OnCommitted(Text, CommitType);
					})
			];
	}

	inline TSharedRef<SWidget> MakePrimaryButton(const FText& Label, FOnClicked OnClicked)
	{
		return SNew(SButton)
			.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
			.ButtonColorAndOpacity(C::Primary)
			.OnClicked(OnClicked)
			.ContentPadding(FMargin(0.0f, 10.0f))
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(ButtonFont())
				.ColorAndOpacity(C::BtnText)
				.Justification(ETextJustify::Center)
			];
	}

	inline TSharedRef<SWidget> MakeSecondaryButton(const FText& Label, FOnClicked OnClicked)
	{
		return SNew(SButton)
			.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
			.ButtonColorAndOpacity(C::Secondary)
			.OnClicked(OnClicked)
			.ContentPadding(FMargin(0.0f, 12.0f))
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(SmallFont())
				.ColorAndOpacity(C::Body)
				.Justification(ETextJustify::Center)
			];
	}

	// =========================================================================
	// Vampire atmospheric background
	// 순수 Slate 레이어로 구성된 다크 고딕 배경:
	//   - 거의 검정에 가까운 베이스
	//   - 상단/하단 크림슨 비네트 (blood fog)
	//   - 좌우 엣지 다크 섀도우
	// =========================================================================
	inline TSharedRef<SWidget> MakeVampireBackground(
		TSharedRef<SWidget> Content,
		EHorizontalAlignment HAlign = HAlign_Center,
		EVerticalAlignment   VAlign = VAlign_Center)
	{
		return SNew(SOverlay)

		// Layer 0: 순수 검정 베이스
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FlatBrush())
			.BorderBackgroundColor(C::ScreenBg)
		]

		// Layer 1: 상단 크림슨 안개 (캐릭터 아우라 느낌)
		+ SOverlay::Slot()
		.VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(240.0f)
			[
				SNew(SBorder)
				.BorderImage(FlatBrush())
				.BorderBackgroundColor(FLinearColor(0.28f, 0.01f, 0.02f, 0.22f))
			]
		]

		// Layer 2: 하단 혈안개 (blood mist)
		+ SOverlay::Slot()
		.VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(180.0f)
			[
				SNew(SBorder)
				.BorderImage(FlatBrush())
				.BorderBackgroundColor(FLinearColor(0.32f, 0.01f, 0.02f, 0.20f))
			]
		]

		// Layer 3: 좌측 엣지 그림자
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		[
			SNew(SBox).WidthOverride(110.0f)
			[
				SNew(SBorder)
				.BorderImage(FlatBrush())
				.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.42f))
			]
		]

		// Layer 4: 우측 엣지 그림자
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		[
			SNew(SBox).WidthOverride(110.0f)
			[
				SNew(SBorder)
				.BorderImage(FlatBrush())
				.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.42f))
			]
		]

		// Layer 5: 중앙 미묘한 크림슨 글로우 (심장부)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(900.0f)
			.HeightOverride(700.0f)
			[
				SNew(SBorder)
				.BorderImage(FlatBrush())
				.BorderBackgroundColor(FLinearColor(0.20f, 0.01f, 0.01f, 0.06f))
			]
		]

		// Content
		+ SOverlay::Slot()
		.HAlign(HAlign)
		.VAlign(VAlign)
		[
			Content
		];
	}
}
