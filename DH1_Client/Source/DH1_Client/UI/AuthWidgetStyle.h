#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
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
		inline const FLinearColor CardBg       = FLinearColor(0.030f, 0.034f, 0.045f, 0.52f);
		inline const FLinearColor CardBorder   = FLinearColor(0.86f,  0.90f,  1.00f,  0.34f);

		// Crimson accent (vampire theme)
		inline const FLinearColor Crimson      = FLinearColor(0.82f,  0.85f,  0.92f,  0.92f);
		inline const FLinearColor CrimsonDim   = FLinearColor(0.72f,  0.75f,  0.85f,  0.24f);

		// Input
		inline const FLinearColor InputBg      = FLinearColor(0.09f, 0.12f, 0.20f, 0.76f);
		inline const FLinearColor InputFg      = FLinearColor(0.90f,  0.88f,  0.85f,  1.0f);
		inline const FLinearColor InputHint    = FLinearColor(0.38f,  0.35f,  0.35f,  1.0f);

		// Primary button — blood red
		inline const FLinearColor Primary      = FLinearColor(0.20f, 0.24f, 0.33f, 0.82f);
		inline const FLinearColor PriHover     = FLinearColor(0.28f, 0.33f, 0.45f, 0.88f);
		inline const FLinearColor PriPress     = FLinearColor(0.14f, 0.17f, 0.24f, 0.90f);

		// Secondary button — dark charcoal
		inline const FLinearColor Secondary    = FLinearColor(0.16f, 0.19f, 0.27f, 0.76f);
		inline const FLinearColor SecHover     = FLinearColor(0.22f, 0.26f, 0.35f, 0.84f);
		inline const FLinearColor SecPress     = FLinearColor(0.12f, 0.15f, 0.21f, 0.90f);

		// Text
		inline const FLinearColor Title        = FLinearColor(0.90f, 0.88f, 0.86f, 1.0f);  // cool off-white
		inline const FLinearColor Body         = FLinearColor(0.82f, 0.84f, 0.90f, 1.0f);
		inline const FLinearColor Dim          = FLinearColor(0.66f, 0.69f, 0.76f, 1.0f);
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

	inline const FSlateBrush* GlassCardBorderBrush()
	{
		static FSlateBrush Brush;
		static bool bInitialized = false;
		if (!bInitialized)
		{
			bInitialized = true;
			Brush.SetResourceObject(nullptr);
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Brush.OutlineSettings.CornerRadii = FVector4(12.0f, 12.0f, 12.0f, 12.0f);
			Brush.OutlineSettings.Width = 1.2f;
			Brush.OutlineSettings.Color = FLinearColor(0.86f, 0.90f, 1.0f, 0.34f);
			Brush.OutlineSettings.bUseBrushTransparency = true;
		}
		return &Brush;
	}

	inline const FSlateBrush* GlassCardFillBrush()
	{
		static FSlateBrush Brush;
		static bool bInitialized = false;
		if (!bInitialized)
		{
			bInitialized = true;
			Brush.SetResourceObject(nullptr);
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Brush.OutlineSettings.CornerRadii = FVector4(10.0f, 10.0f, 10.0f, 10.0f);
		}
		return &Brush;
	}

	// 로그인 배경 이미지 브러시
	// 우선 /Game/UI/T_LoginBg 에셋을 사용하고, 실패 시 PNG 파일을 디코드합니다.
	// PNG로 생성한 텍스처는 GC로 사라지지 않도록 Root에 고정합니다.
	inline const FSlateBrush* GetLoginBgBrush()
	{
		auto LoadTextureFromPngOrAsset = []() -> UTexture2D*
		{
			// 1) Primary: decode raw PNG from Content/UI/T_LoginBg.png
			const FString FilePath = FPaths::ProjectContentDir() / TEXT("UI/T_LoginBg.png");
			TArray<uint8> RawData;
			if (!FFileHelper::LoadFileToArray(RawData, *FilePath))
			{
				// 2) Fallback: imported uasset
				return LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/T_LoginBg.T_LoginBg"));
			}

			IImageWrapperModule& WrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
			const EImageFormat DetectedFormat = WrapperModule.DetectImageFormat(RawData.GetData(), RawData.Num());
			TSharedPtr<IImageWrapper> Wrapper = DetectedFormat != EImageFormat::Invalid
				? WrapperModule.CreateImageWrapper(DetectedFormat)
				: nullptr;
			if (!Wrapper.IsValid() || !Wrapper->SetCompressed(RawData.GetData(), RawData.Num()))
			{
				return LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/T_LoginBg.T_LoginBg"));
			}

			TArray64<uint8> Pixels;
			if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, Pixels))
			{
				return LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/T_LoginBg.T_LoginBg"));
			}

			const int32 W = Wrapper->GetWidth();
			const int32 H = Wrapper->GetHeight();
			UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
			if (!Tex || !Tex->GetPlatformData() || Tex->GetPlatformData()->Mips.Num() == 0)
			{
				return LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/T_LoginBg.T_LoginBg"));
			}

			void* MipData = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(MipData, Pixels.GetData(), Pixels.Num());
			Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
			Tex->UpdateResource();
			Tex->AddToRoot();
			return Tex;
		};

		static FSlateBrush BgBrush;
		static bool bBrushInit = false;
		static TWeakObjectPtr<UTexture2D> CachedTexture;
		if (!bBrushInit)
		{
			bBrushInit = true;
			BgBrush.DrawAs = ESlateBrushDrawType::Image;
			BgBrush.Tiling = ESlateBrushTileType::NoTile;
			BgBrush.ImageType = ESlateBrushImageType::FullColor;
		}

		// If previous attempt failed (or resource got invalid), retry load.
		if (!CachedTexture.IsValid())
		{
			CachedTexture = LoadTextureFromPngOrAsset();
		}

		if (UTexture2D* Texture = CachedTexture.Get())
		{
			BgBrush.SetResourceObject(Texture);
			BgBrush.ImageSize = FVector2D(
				static_cast<float>(Texture->GetSizeX()),
				static_cast<float>(Texture->GetSizeY()));
		}
		else
		{
			// Keep Slate from touching stale UObject pointers.
			BgBrush.SetResourceObject(nullptr);
		}

		return &BgBrush;
	}

	// =========================================================================
	// Button style
	// =========================================================================
	inline FButtonStyle PrimaryButtonStyle()
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.Normal.SetResourceObject(nullptr);
		Style.Hovered.SetResourceObject(nullptr);
		Style.Pressed.SetResourceObject(nullptr);
		Style.Disabled.SetResourceObject(nullptr);
		Style.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.Disabled.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.Normal.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.Hovered.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.Pressed.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.Disabled.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.Normal.OutlineSettings.Color = FLinearColor(0.86f, 0.90f, 1.0f, 0.30f);
		Style.Hovered.OutlineSettings.Color = FLinearColor(0.92f, 0.95f, 1.0f, 0.40f);
		Style.Pressed.OutlineSettings.Color = FLinearColor(0.78f, 0.83f, 0.94f, 0.38f);
		Style.Disabled.OutlineSettings.Color = FLinearColor(0.72f, 0.75f, 0.85f, 0.20f);
		Style.Normal.TintColor = FSlateColor(C::Primary);
		Style.Hovered.TintColor = FSlateColor(C::PriHover);
		Style.Pressed.TintColor = FSlateColor(C::PriPress);
		return Style;
	}

	inline FButtonStyle SecondaryButtonStyle()
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.Normal.SetResourceObject(nullptr);
		Style.Hovered.SetResourceObject(nullptr);
		Style.Pressed.SetResourceObject(nullptr);
		Style.Disabled.SetResourceObject(nullptr);
		Style.Normal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.Hovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.Pressed.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.Disabled.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.Normal.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.Hovered.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.Pressed.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.Disabled.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.Normal.OutlineSettings.Color = FLinearColor(0.80f, 0.84f, 0.95f, 0.24f);
		Style.Hovered.OutlineSettings.Color = FLinearColor(0.88f, 0.92f, 1.0f, 0.33f);
		Style.Pressed.OutlineSettings.Color = FLinearColor(0.72f, 0.76f, 0.88f, 0.30f);
		Style.Disabled.OutlineSettings.Color = FLinearColor(0.66f, 0.70f, 0.80f, 0.18f);
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
		Style.BackgroundImageNormal.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.BackgroundImageHovered.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.BackgroundImageFocused.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.BackgroundImageReadOnly.DrawAs = ESlateBrushDrawType::RoundedBox;
		Style.BackgroundImageNormal.SetResourceObject(nullptr);
		Style.BackgroundImageHovered.SetResourceObject(nullptr);
		Style.BackgroundImageFocused.SetResourceObject(nullptr);
		Style.BackgroundImageReadOnly.SetResourceObject(nullptr);
		Style.BackgroundImageNormal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.BackgroundImageHovered.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.BackgroundImageFocused.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.BackgroundImageReadOnly.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Style.BackgroundImageNormal.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.BackgroundImageHovered.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.BackgroundImageFocused.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.BackgroundImageReadOnly.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Style.BackgroundImageNormal.OutlineSettings.Color = FLinearColor(0.84f, 0.88f, 0.98f, 0.26f);
		Style.BackgroundImageHovered.OutlineSettings.Color = FLinearColor(0.90f, 0.94f, 1.0f, 0.36f);
		Style.BackgroundImageFocused.OutlineSettings.Color = FLinearColor(0.95f, 0.98f, 1.0f, 0.46f);
		Style.BackgroundImageReadOnly.OutlineSettings.Color = FLinearColor(0.74f, 0.78f, 0.90f, 0.20f);
		Style.BackgroundImageNormal.TintColor = FSlateColor(C::InputBg);
		Style.BackgroundImageHovered.TintColor = FSlateColor(FLinearColor(0.12f, 0.16f, 0.25f, 0.84f));
		Style.BackgroundImageFocused.TintColor = FSlateColor(FLinearColor(0.15f, 0.20f, 0.30f, 0.88f));
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
		static const FButtonStyle PrimaryStyle = PrimaryButtonStyle();
		return SNew(SButton)
			.ButtonStyle(&PrimaryStyle)
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
		static const FButtonStyle SecondaryStyle = SecondaryButtonStyle();
		return SNew(SButton)
			.ButtonStyle(&SecondaryStyle)
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
	// Atmospheric background
	// 배경 이미지를 그대로 보여주고, 콘텐츠 배치만 제어합니다.
	// =========================================================================
	inline TSharedRef<SWidget> MakeVampireBackground(
		TSharedRef<SWidget> Content,
		EHorizontalAlignment HAlign = HAlign_Center,
		EVerticalAlignment   VAlign = VAlign_Center,
		const FMargin& ContentPadding = FMargin(0.0f))
	{
		return SNew(SOverlay)

		// Layer 0: 배경 이미지 (Content/UI/T_LoginBg 텍스처)
		// 텍스처 미임포트 시 빈 브러시로 렌더링됩니다.
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(GetLoginBgBrush())
		]

		// Content
		+ SOverlay::Slot()
		.HAlign(HAlign)
		.VAlign(VAlign)
		.Padding(ContentPadding)
		[
			Content
		];
	}
}
