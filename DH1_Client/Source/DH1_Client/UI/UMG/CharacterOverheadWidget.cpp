#include "CharacterOverheadWidget.h"

#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
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
			Style.FillImage.SetResourceObject(nullptr);
			Style.FillImage.DrawAs = ESlateBrushDrawType::RoundedBox;
			Style.FillImage.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Style.FillImage.OutlineSettings.CornerRadii = FVector4(5.f, 5.f, 5.f, 5.f);
			Style.FillImage.TintColor = FSlateColor(FLinearColor(0.55f, 0.16f, 0.22f, 0.92f));
			Style.BackgroundImage = *FCoreStyle::Get().GetBrush("WhiteBrush");
			Style.BackgroundImage.SetResourceObject(nullptr);
			Style.BackgroundImage.DrawAs = ESlateBrushDrawType::RoundedBox;
			Style.BackgroundImage.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Style.BackgroundImage.OutlineSettings.CornerRadii = FVector4(5.f, 5.f, 5.f, 5.f);
			Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.10f, 0.12f, 0.18f, 0.38f));
		}
		return Style;
	}

	void InitRoundedBrush(FSlateBrush& Brush, const FLinearColor& Fill, const FLinearColor& Outline,
		const float Radius, const float OutlineWidth = 1.f)
	{
		Brush = FSlateBrush();
		Brush.SetResourceObject(nullptr);
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		Brush.OutlineSettings.Width = OutlineWidth;
		Brush.OutlineSettings.Color = Outline;
		Brush.OutlineSettings.bUseBrushTransparency = true;
		Brush.TintColor = FSlateColor(Fill);
	}

	/** 바깥 링: 얇은 하이라이트 테두리 + 거의 투명한 채움 (글래스 외곽) */
	const FSlateBrush* GetOverheadOuterBrush()
	{
		static FSlateBrush B;
		static bool bOnce = false;
		if (!bOnce)
		{
			bOnce = true;
			InitRoundedBrush(
				B,
				FLinearColor(0.10f, 0.14f, 0.22f, 0.14f),
				FLinearColor(0.88f, 0.92f, 1.00f, 0.42f),
				14.f,
				1.15f);
		}
		return &B;
	}

	/** 안쪽 패널: 반투명 냉색 글래스 + 은은한 내부 림 */
	const FSlateBrush* GetOverheadInnerBrush()
	{
		static FSlateBrush B;
		static bool bOnce = false;
		if (!bOnce)
		{
			bOnce = true;
			InitRoundedBrush(
				B,
				FLinearColor(0.06f, 0.09f, 0.14f, 0.48f),
				FLinearColor(0.72f, 0.80f, 0.96f, 0.22f),
				11.f,
				0.85f);
		}
		return &B;
	}

	const FSlateBrush* GetLevelBadgeBrush()
	{
		static FSlateBrush B;
		static bool bOnce = false;
		if (!bOnce)
		{
			bOnce = true;
			InitRoundedBrush(
				B,
				FLinearColor(0.12f, 0.16f, 0.24f, 0.42f),
				FLinearColor(0.82f, 0.88f, 1.00f, 0.35f),
				8.f,
				0.9f);
		}
		return &B;
	}
}

float UCharacterOverheadWidget::GetHealthPercent() const
{
	return CachedMaxHP > 0.f ? FMath::Clamp(CachedCurrentHP / CachedMaxHP, 0.f, 1.f) : 0.f;
}

TSharedRef<SWidget> UCharacterOverheadWidget::RebuildWidget()
{
	using namespace GameStyle;

	return SNew(SBox)
		.MinDesiredWidth(272.f)
		.Padding(FMargin(3.f, 5.f))
		[
			SNew(SBorder)
			.BorderImage(GetOverheadOuterBrush())
			.Padding(FMargin(4.f))
			[
				SNew(SBorder)
				.BorderImage(GetOverheadInnerBrush())
				.Padding(FMargin(13.f, 11.f, 13.f, 12.f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 0.f, 0.f, 8.f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(SSpacer)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.f, 0.f, 10.f, 0.f)
						[
							SNew(SBorder)
							.BorderImage(GetLevelBadgeBrush())
							.Padding(FMargin(8.f, 3.f, 8.f, 3.f))
							[
								SAssignNew(SlateLevelText, STextBlock)
								.Text(FText::Format(NSLOCTEXT("DH1", "OverheadLevelFmt", "Lv. {0}"), FText::AsNumber(CachedLevel)))
								.Font(GetOverheadTextFont(12))
								.ColorAndOpacity(FLinearColor(0.78f, 0.82f, 0.92f, 1.f))
								.Justification(ETextJustify::Center)
							]
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SAssignNew(SlateNameText, STextBlock)
							.Text(FText::FromString(CachedDisplayName))
							.Font(GetOverheadTextFont(16))
							.ColorAndOpacity(FLinearColor(0.94f, 0.95f, 0.98f, 1.f))
							.Justification(ETextJustify::Center)
						]

						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(SSpacer)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 5.f)
					[
						SNew(SBox)
						.HeightOverride(7.f)
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
						.ColorAndOpacity(FLinearColor(0.52f, 0.56f, 0.62f, 0.95f))
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
		const FLinearColor NameColor = bCachedIsLocal
			? FLinearColor(0.0f, 1.0f, 0.4f, 1.f)
			: FLinearColor(1.0f, 0.35f, 0.2f, 1.f);
		SlateNameText->SetColorAndOpacity(NameColor);
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

void UCharacterOverheadWidget::SetIsLocalPlayer(const bool bLocal)
{
	bCachedIsLocal = bLocal;
	RefreshOverheadSlate();
}
