#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

// ============================================================================
// GameStyle — 인게임 HUD/위젯 공통 스타일
//
// 모든 인게임 패널(인벤토리, 채팅, 스킬바, 퀘스트 등)에 적용할 다크 테마.
// 반투명 검정/회색 배경 + 흰색 텍스트.
// ============================================================================

namespace GameStyle
{
	// =========================================================================
	// Color Palette  (Dark Transparent Theme)
	// =========================================================================
	namespace C
	{
		// 패널 배경 — 반투명 검정
		inline const FLinearColor PanelBg      = FLinearColor(0.00f, 0.00f, 0.00f, 0.75f);
		inline const FLinearColor PanelBorder  = FLinearColor(0.30f, 0.30f, 0.30f, 0.55f);
		inline const FLinearColor HeaderBg     = FLinearColor(0.04f, 0.04f, 0.04f, 0.88f);

		// 항목 상태
		inline const FLinearColor ItemHover    = FLinearColor(0.10f, 0.10f, 0.10f, 0.80f);
		inline const FLinearColor ItemSelected = FLinearColor(0.15f, 0.15f, 0.15f, 0.90f);

		// 텍스트
		inline const FLinearColor TextPrimary   = FLinearColor(1.00f, 1.00f, 1.00f, 1.0f);  // 흰색
		inline const FLinearColor TextSecondary = FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);  // 밝은 회색
		inline const FLinearColor TextDim       = FLinearColor(0.50f, 0.50f, 0.50f, 1.0f);  // 흐린 회색

		// 구분선
		inline const FLinearColor Separator    = FLinearColor(0.40f, 0.40f, 0.40f, 0.35f);

		// 버튼
		inline const FLinearColor ButtonBg     = FLinearColor(0.12f, 0.12f, 0.12f, 0.90f);
		inline const FLinearColor ButtonHover  = FLinearColor(0.22f, 0.22f, 0.22f, 0.90f);
		inline const FLinearColor ButtonPress  = FLinearColor(0.06f, 0.06f, 0.06f, 0.95f);
		inline const FLinearColor ButtonText   = FLinearColor(1.00f, 1.00f, 1.00f, 1.0f);
	}

	// =========================================================================
	// Fonts
	// =========================================================================
	inline FSlateFontInfo TitleFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", 16);
	}

	inline FSlateFontInfo BodyFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", 14);
	}

	inline FSlateFontInfo SmallFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", 12);
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
	inline FButtonStyle GameButtonStyle()
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.Normal.TintColor  = FSlateColor(C::ButtonBg);
		Style.Hovered.TintColor = FSlateColor(C::ButtonHover);
		Style.Pressed.TintColor = FSlateColor(C::ButtonPress);
		return Style;
	}

	// =========================================================================
	// MakeGamePanel
	// 반투명 검정 배경 + 회색 1px 테두리 + 흰색 헤더 타이틀로 구성된
	// 표준 인게임 패널 레이아웃을 반환합니다.
	//
	// 사용 예:
	//   ChildSlot[ GameStyle::MakeGamePanel(MyContent, INVTEXT("인벤토리")) ];
	// =========================================================================
	inline TSharedRef<SWidget> MakeGamePanel(TSharedRef<SWidget> Content, const FText& Title = FText::GetEmpty())
	{
		const bool bHasTitle = !Title.IsEmpty();

		return SNew(SBorder)
			.BorderImage(FlatBrush())
			.BorderBackgroundColor(C::PanelBorder)
			.Padding(1.0f)
			[
				SNew(SVerticalBox)

				// 헤더 (타이틀이 있을 때만)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.Visibility(bHasTitle ? EVisibility::Visible : EVisibility::Collapsed)
					.BorderImage(FlatBrush())
					.BorderBackgroundColor(C::HeaderBg)
					.Padding(FMargin(10.0f, 6.0f))
					[
						SNew(STextBlock)
						.Text(Title)
						.Font(TitleFont())
						.ColorAndOpacity(C::TextPrimary)
					]
				]

				// 내용 영역
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SBorder)
					.BorderImage(FlatBrush())
					.BorderBackgroundColor(C::PanelBg)
					.Padding(FMargin(8.0f))
					[
						Content
					]
				]
			];
	}
}
