#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SBoxPanel.h"

namespace AuthStyle
{
	// =========================================================================
	// Color Palette
	// =========================================================================
	namespace C
	{
		// Background
		inline const FLinearColor ScreenBg     = FLinearColor(0.01f, 0.01f, 0.02f, 0.97f);
		inline const FLinearColor CardBg       = FLinearColor(0.04f, 0.04f, 0.06f, 1.0f);

		// Input
		inline const FLinearColor InputBg      = FLinearColor(0.06f, 0.06f, 0.08f, 1.0f);
		inline const FLinearColor InputFg      = FLinearColor(0.92f, 0.92f, 0.95f, 1.0f);
		inline const FLinearColor InputHint    = FLinearColor(0.40f, 0.40f, 0.45f, 1.0f);

		// Primary button
		inline const FLinearColor Primary      = FLinearColor(0.15f, 0.45f, 0.85f, 1.0f);
		inline const FLinearColor PriHover     = FLinearColor(0.20f, 0.55f, 0.95f, 1.0f);
		inline const FLinearColor PriPress     = FLinearColor(0.10f, 0.35f, 0.70f, 1.0f);

		// Secondary button
		inline const FLinearColor Secondary    = FLinearColor(0.12f, 0.12f, 0.15f, 1.0f);
		inline const FLinearColor SecHover     = FLinearColor(0.18f, 0.18f, 0.22f, 1.0f);
		inline const FLinearColor SecPress     = FLinearColor(0.08f, 0.08f, 0.10f, 1.0f);

		// Text
		inline const FLinearColor Title        = FLinearColor(0.95f, 0.95f, 0.97f, 1.0f);
		inline const FLinearColor Body         = FLinearColor(0.78f, 0.78f, 0.82f, 1.0f);
		inline const FLinearColor Dim          = FLinearColor(0.45f, 0.45f, 0.50f, 1.0f);
		inline const FLinearColor Error        = FLinearColor(0.95f, 0.30f, 0.30f, 1.0f);
		inline const FLinearColor Success      = FLinearColor(0.20f, 0.90f, 0.40f, 1.0f);
		inline const FLinearColor BtnText      = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
}
