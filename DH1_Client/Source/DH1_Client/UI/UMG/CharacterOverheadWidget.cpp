#include "CharacterOverheadWidget.h"

#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "UI/GameWidgetStyle.h"

namespace
{
	/** Roboto 기본 폰트는 CJK 글리프가 없어 한글 이름이 비어 보일 수 있음 — 엔진 Slate 폴백 폰트 우선. */
	FSlateFontInfo GetOverheadTextFont(const int32 Size)
	{
		static FString CachedPath;
		static bool bTriedResolve = false;
		if (!bTriedResolve)
		{
			bTriedResolve = true;
			IPlatformFile& Pf = FPlatformFileManager::Get().GetPlatformFile();
			const FString Base = FPaths::EngineContentDir() / TEXT("Slate");
			static const TCHAR* const Candidates[] = {
				TEXT("Fonts/DroidSansFallback.ttf"),
				TEXT("Fonts/NotoSansCJKsc-Regular.otf"),
				TEXT("Fonts/NotoSansCJK-Regular.otf"),
				TEXT("Fonts/NotoSansKR-Regular.otf"),
			};
			for (const TCHAR* Rel : Candidates)
			{
				const FString Full = FPaths::Combine(Base, Rel);
				if (Pf.FileExists(*Full))
				{
					CachedPath = Full;
					break;
				}
			}
		}
		if (!CachedPath.IsEmpty())
		{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
			const FSlateFontInfo Info(CachedPath, Size);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
			return Info;
		}
		return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
	}

	const FProgressBarStyle& GetOverheadHealthBarStyle()
	{
		static FProgressBarStyle Style = FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>("ProgressBar");
		static bool bInited = false;
		if (!bInited)
		{
			bInited = true;
			Style.FillImage = *FCoreStyle::Get().GetBrush("WhiteBrush");
			// 짙은 빨강 (밝은 코랄 톤보다 채도·어두움 유지)
			Style.FillImage.TintColor = FSlateColor(FLinearColor(0.40f, 0.04f, 0.06f, 1.f));
			Style.BackgroundImage = *FCoreStyle::Get().GetBrush("WhiteBrush");
			Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.07f, 0.07f, 0.09f));
		}
		return Style;
	}
}

float UCharacterOverheadWidget::GetHealthPercent() const
{
	return CachedMaxHP > 0.f ? FMath::Clamp(CachedCurrentHP / CachedMaxHP, 0.f, 1.f) : 0.f;
}

TSharedRef<SWidget> UCharacterOverheadWidget::RebuildWidget()
{
	using namespace GameStyle;

	const FSlateBrush* const WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	return SNew(SBox)
		.MinDesiredWidth(228.f)
		.Padding(4.f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(C::PanelBorder)
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.88f))
				.Padding(FMargin(12.f, 10.f, 12.f, 10.f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(SlateNameText, STextBlock)
						.Text(FText::FromString(CachedDisplayName))
						.Font(GetOverheadTextFont(15))
						.ColorAndOpacity(C::TextPrimary)
						.Justification(ETextJustify::Center)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 8.f, 0.f, 8.f)
					[
						SNew(SBox)
						.HeightOverride(1.f)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(C::Separator)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SAssignNew(SlateLevelText, STextBlock)
						.Text(FText::Format(NSLOCTEXT("DH1", "OverheadLevelFmt", "Lv. {0}"), FText::AsNumber(CachedLevel)))
						.Font(GetOverheadTextFont(12))
						.ColorAndOpacity(C::TextSecondary)
						.Justification(ETextJustify::Center)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(C::HealthBarTrack)
						.Padding(FMargin(2.f))
						[
							SAssignNew(SlateHealthBar, SProgressBar)
							.Style(&GetOverheadHealthBarStyle())
							.Percent(GetHealthPercent())
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(SlateHPText, STextBlock)
						.Text(FText::Format(
							NSLOCTEXT("DH1", "OverheadHPFmt", "{0} / {1}"),
							FText::AsNumber(FMath::RoundToInt(CachedCurrentHP)),
							FText::AsNumber(FMath::RoundToInt(CachedMaxHP))))
						.Font(GetOverheadTextFont(11))
						.ColorAndOpacity(C::TextDim)
						.Justification(ETextJustify::Center)
					]
				]
			]
		];
}

void UCharacterOverheadWidget::RefreshOverheadSlate()
{
	if (SlateNameText.IsValid())
	{
		SlateNameText->SetText(FText::FromString(CachedDisplayName));
	}
	if (SlateLevelText.IsValid())
	{
		SlateLevelText->SetText(FText::Format(NSLOCTEXT("DH1", "OverheadLevelFmt", "Lv. {0}"), FText::AsNumber(CachedLevel)));
	}
	if (SlateHPText.IsValid())
	{
		SlateHPText->SetText(FText::Format(
			NSLOCTEXT("DH1", "OverheadHPFmt", "{0} / {1}"),
			FText::AsNumber(FMath::RoundToInt(CachedCurrentHP)),
			FText::AsNumber(FMath::RoundToInt(CachedMaxHP))));
	}
	if (SlateHealthBar.IsValid())
	{
		SlateHealthBar->SetPercent(GetHealthPercent());
	}

	if (const TSharedPtr<SWidget> Root = GetCachedWidget())
	{
		Root->Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
	}
}

void UCharacterOverheadWidget::SetDisplayName(const FString& InName)
{
	FString Trimmed = InName;
	Trimmed.TrimStartAndEndInline();
	CachedDisplayName = Trimmed.IsEmpty() ? TEXT("Adventurer") : Trimmed;
	RefreshOverheadSlate();
}

void UCharacterOverheadWidget::SetLevel(const int32 InLevel)
{
	CachedLevel = FMath::Clamp(FMath::Max(1, InLevel), 1, 9999);
	RefreshOverheadSlate();
}

void UCharacterOverheadWidget::SetHealth(const float Current, const float Max)
{
	CachedMaxHP = FMath::Max(1.f, Max);
	CachedCurrentHP = FMath::Clamp(Current, 0.f, CachedMaxHP);
	RefreshOverheadSlate();
}

void UCharacterOverheadWidget::SetOverheadData(const FString& InName, const int32 InLevel, const float CurrentHP, const float MaxHP)
{
	FString Trimmed = InName;
	Trimmed.TrimStartAndEndInline();
	CachedDisplayName = Trimmed.IsEmpty() ? TEXT("Adventurer") : Trimmed;
	CachedLevel = FMath::Clamp(FMath::Max(1, InLevel), 1, 9999);
	CachedMaxHP = FMath::Max(1.f, MaxHP);
	CachedCurrentHP = FMath::Clamp(CurrentHP, 0.f, CachedMaxHP);
	RefreshOverheadSlate();
}
