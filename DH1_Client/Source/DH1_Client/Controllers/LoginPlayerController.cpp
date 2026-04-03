#include "Controllers/LoginPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UI/SAuthMasterWidget.h"

namespace
{
	void SetNamedWidgetsVisibility(UUserWidget* RootWidget, const TArray<FString>& NameKeywords, ESlateVisibility Visibility)
	{
		if (!RootWidget || !RootWidget->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> AllWidgets;
		RootWidget->WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* Widget : AllWidgets)
		{
			if (!Widget)
			{
				continue;
			}

			const FString Name = Widget->GetName().ToLower();
			for (const FString& Keyword : NameKeywords)
			{
				if (Name.Contains(Keyword))
				{
					Widget->SetVisibility(Visibility);
					break;
				}
			}
		}
	}

	void SetEditableTextBoxesVisibilityByName(
		UUserWidget* RootWidget,
		const TArray<FString>& NameKeywords,
		const ESlateVisibility Visibility)
	{
		if (!RootWidget || !RootWidget->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> AllWidgets;
		RootWidget->WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* Widget : AllWidgets)
		{
			UEditableTextBox* TextBox = Cast<UEditableTextBox>(Widget);
			if (!TextBox)
			{
				continue;
			}

			const FString Name = TextBox->GetName().ToLower();
			for (const FString& Keyword : NameKeywords)
			{
				if (Name.Contains(Keyword))
				{
					TextBox->SetVisibility(Visibility);
					TextBox->SetIsEnabled(true);
					TextBox->SetIsReadOnly(false);
					break;
				}
			}
		}
	}

	int32 CountVisibleEditableTextBoxes(UUserWidget* RootWidget)
	{
		if (!RootWidget || !RootWidget->WidgetTree)
		{
			return 0;
		}

		int32 VisibleCount = 0;
		TArray<UWidget*> AllWidgets;
		RootWidget->WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* Widget : AllWidgets)
		{
			UEditableTextBox* TextBox = Cast<UEditableTextBox>(Widget);
			if (!TextBox)
			{
				continue;
			}

			const ESlateVisibility Visibility = TextBox->GetVisibility();
			if (Visibility == ESlateVisibility::Visible || Visibility == ESlateVisibility::HitTestInvisible ||
				Visibility == ESlateVisibility::SelfHitTestInvisible)
			{
				++VisibleCount;
			}
		}

		return VisibleCount;
	}

	void EnsureFallbackEditableTextBoxesVisible(UUserWidget* RootWidget)
	{
		if (!RootWidget || !RootWidget->WidgetTree)
		{
			return;
		}

		if (CountVisibleEditableTextBoxes(RootWidget) > 0)
		{
			return;
		}

		TArray<UWidget*> AllWidgets;
		RootWidget->WidgetTree->GetAllWidgets(AllWidgets);

		int32 RestoredCount = 0;
		for (UWidget* Widget : AllWidgets)
		{
			UEditableTextBox* TextBox = Cast<UEditableTextBox>(Widget);
			if (!TextBox)
			{
				continue;
			}

			TextBox->SetVisibility(ESlateVisibility::Visible);
			TextBox->SetIsEnabled(true);
			TextBox->SetIsReadOnly(false);
			++RestoredCount;

			// Legacy login UMG usually reuses up to 2~4 text boxes.
			if (RestoredCount >= 4)
			{
				break;
			}
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("LoginPlayerController: Restored %d editable text box(es) via fallback visibility recovery"),
			RestoredCount);
	}

	void ApplyDarkMmorpgStyle(UUserWidget* RootWidget, ALoginPlayerController* OwnerController)
	{
		if (!RootWidget || !RootWidget->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> AllWidgets;
		RootWidget->WidgetTree->GetAllWidgets(AllWidgets);

		const FLinearColor PanelDark(0.01f, 0.01f, 0.01f, 0.62f);
		const FLinearColor PanelDarkSoft(0.02f, 0.02f, 0.02f, 0.46f);
		const FSlateColor BodyText(FLinearColor(0.90f, 0.90f, 0.90f, 1.0f));
		const FSlateColor SecondaryText(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));
		const float FormCenterYOffset = 6.0f; // Fine-tune: slightly lower, cleaner vertical rhythm.

		const auto ApplyCenteredCanvasLayout = [FormCenterYOffset](UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
		{
			if (!Widget)
			{
				return;
			}
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
			{
				CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetPosition(FVector2D(Position.X, Position.Y + FormCenterYOffset));
				CanvasSlot->SetSize(Size);
				CanvasSlot->SetAutoSize(false);
			}
		};

		const auto BuildActionButtonStyle = [PanelDarkSoft]()
		{
			FButtonStyle Style;
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
			Style.Normal.TintColor = FSlateColor(FLinearColor(0.12f, 0.14f, 0.18f, 0.72f));
			Style.Hovered.TintColor = FSlateColor(FLinearColor(0.18f, 0.21f, 0.28f, 0.82f));
			Style.Pressed.TintColor = FSlateColor(FLinearColor(0.10f, 0.12f, 0.16f, 0.88f));
			Style.Disabled.TintColor = FSlateColor(PanelDarkSoft);
			Style.Normal.OutlineSettings.Color = FLinearColor(0.88f, 0.90f, 0.98f, 0.34f);
			Style.Hovered.OutlineSettings.Color = FLinearColor(0.95f, 0.96f, 1.0f, 0.44f);
			return Style;
		};

		for (UWidget* Child : AllWidgets)
		{
			const FString Name = Child->GetName().ToLower();

			if (UImage* Image = Cast<UImage>(Child))
			{
				if (Name == TEXT("img_background"))
				{
					Image->SetVisibility(ESlateVisibility::Hidden);
					Image->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
					FSlateBrush Brush = Image->GetBrush();
					Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
					Brush.SetResourceObject(nullptr);
					Image->SetBrush(Brush);
					continue;
				}
				if (Name == TEXT("img_panelborder"))
				{
					FSlateBrush Brush = Image->GetBrush();
					Brush.SetResourceObject(nullptr);
					Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
					Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
					Brush.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
					Brush.OutlineSettings.Width = 1.2f;
					Brush.OutlineSettings.Color = FLinearColor(0.86f, 0.90f, 1.0f, 0.33f);
					Brush.OutlineSettings.bUseBrushTransparency = true;
					Image->SetBrush(Brush);
					Image->SetColorAndOpacity(FLinearColor(0.18f, 0.20f, 0.24f, 0.30f));
					ApplyCenteredCanvasLayout(Image, FVector2D(0.0f, -2.0f), FVector2D(548.0f, 404.0f));
					continue;
				}
				if (Name == TEXT("img_panel"))
				{
					FSlateBrush Brush = Image->GetBrush();
					Brush.SetResourceObject(nullptr);
					Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
					Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
					Brush.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
					Brush.OutlineSettings.Width = 0.8f;
					Brush.OutlineSettings.Color = FLinearColor(0.85f, 0.90f, 1.0f, 0.27f);
					Brush.OutlineSettings.bUseBrushTransparency = true;
					Image->SetBrush(Brush);
					Image->SetColorAndOpacity(FLinearColor(0.07f, 0.08f, 0.11f, 0.56f));
					ApplyCenteredCanvasLayout(Image, FVector2D(0.0f, -2.0f), FVector2D(536.0f, 392.0f));
					continue;
				}
				if (Name == TEXT("img_septop"))
				{
					Image->SetColorAndOpacity(FLinearColor(0.90f, 0.93f, 1.0f, 0.26f));
					ApplyCenteredCanvasLayout(Image, FVector2D(0.0f, -146.0f), FVector2D(430.0f, 1.0f));
					continue;
				}
				if (Name == TEXT("img_sepbottom"))
				{
					Image->SetColorAndOpacity(FLinearColor(0.72f, 0.75f, 0.85f, 0.20f));
					ApplyCenteredCanvasLayout(Image, FVector2D(0.0f, 92.0f), FVector2D(430.0f, 1.0f));
					continue;
				}
			}

			if (UBorder* Border = Cast<UBorder>(Child))
			{
				if (Name.Contains(TEXT("bg")) || Name.Contains(TEXT("background")))
				{
					continue;
				}

				if (Name.Contains(TEXT("panel")) || Name.Contains(TEXT("card")) || Name.Contains(TEXT("box")) ||
					Name.Contains(TEXT("login")) || Name.Contains(TEXT("form")))
				{
					Border->SetBrushColor(PanelDark);
				}
				else
				{
					Border->SetBrushColor(PanelDarkSoft);
				}
				continue;
			}

			if (UEditableTextBox* TextBox = Cast<UEditableTextBox>(Child))
			{
				FEditableTextBoxStyle Style = TextBox->GetWidgetStyle();
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
				Style.BackgroundImageNormal.TintColor = FSlateColor(FLinearColor(0.18f, 0.20f, 0.26f, 0.72f));
				Style.BackgroundImageHovered.TintColor = FSlateColor(FLinearColor(0.24f, 0.27f, 0.34f, 0.80f));
				Style.BackgroundImageFocused.TintColor = FSlateColor(FLinearColor(0.28f, 0.31f, 0.38f, 0.84f));
				Style.BackgroundImageReadOnly.TintColor = FSlateColor(FLinearColor(0.18f, 0.20f, 0.26f, 0.72f));
				Style.ForegroundColor = FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f));
				TextBox->SetWidgetStyle(Style);
				TextBox->SetIsEnabled(true);
				TextBox->SetIsReadOnly(false);
				TextBox->SetRenderOpacity(1.0f);

				if (Name == TEXT("txtbox_username"))
				{
					ApplyCenteredCanvasLayout(TextBox, FVector2D(0.0f, -98.0f), FVector2D(430.0f, 44.0f));
				}
				else if (Name == TEXT("txtbox_password"))
				{
					ApplyCenteredCanvasLayout(TextBox, FVector2D(0.0f, -22.0f), FVector2D(430.0f, 44.0f));
				}
				else if ((Name.Contains(TEXT("signup")) || Name.Contains(TEXT("register"))) && Name.Contains(TEXT("email")))
				{
					ApplyCenteredCanvasLayout(TextBox, FVector2D(0.0f, -102.0f), FVector2D(430.0f, 42.0f));
				}
				else if ((Name.Contains(TEXT("signup")) || Name.Contains(TEXT("register"))) &&
					Name.Contains(TEXT("password")) && !Name.Contains(TEXT("confirm")))
				{
					ApplyCenteredCanvasLayout(TextBox, FVector2D(0.0f, -36.0f), FVector2D(430.0f, 42.0f));
				}
				else if ((Name.Contains(TEXT("signup")) || Name.Contains(TEXT("register"))) &&
					Name.Contains(TEXT("confirm")))
				{
					ApplyCenteredCanvasLayout(TextBox, FVector2D(0.0f, 30.0f), FVector2D(430.0f, 42.0f));
				}
				else if ((Name.Contains(TEXT("reset")) || Name.Contains(TEXT("forgot"))) && Name.Contains(TEXT("email")))
				{
					ApplyCenteredCanvasLayout(TextBox, FVector2D(0.0f, -124.0f), FVector2D(430.0f, 40.0f));
				}
				else if (Name.Contains(TEXT("verify")) && Name.Contains(TEXT("code")))
				{
					ApplyCenteredCanvasLayout(TextBox, FVector2D(0.0f, -70.0f), FVector2D(430.0f, 40.0f));
				}
				else if ((Name.Contains(TEXT("reset")) || Name.Contains(TEXT("new"))) &&
					Name.Contains(TEXT("password")) && !Name.Contains(TEXT("confirm")))
				{
					ApplyCenteredCanvasLayout(TextBox, FVector2D(0.0f, -16.0f), FVector2D(430.0f, 40.0f));
				}
				else if ((Name.Contains(TEXT("reset")) || Name.Contains(TEXT("new"))) &&
					Name.Contains(TEXT("confirm")))
				{
					ApplyCenteredCanvasLayout(TextBox, FVector2D(0.0f, 38.0f), FVector2D(430.0f, 40.0f));
				}
				continue;
			}

			if (UButton* Button = Cast<UButton>(Child))
			{
				FButtonStyle Style = BuildActionButtonStyle();
				Button->SetStyle(Style);
				UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Found button %s"), *Button->GetName());

				if (Name == TEXT("btn_login"))
				{
					Button->SetVisibility(ESlateVisibility::Visible);
					Button->SetRenderOpacity(1.0f);
					ApplyCenteredCanvasLayout(Button, FVector2D(0.0f, 44.0f), FVector2D(230.0f, 46.0f));
				}
				else if (Name.Contains(TEXT("forgot")) || Name.Contains(TEXT("reset")) || Name.Contains(TEXT("changepassword")))
				{
					Button->SetVisibility(ESlateVisibility::Visible);
					Button->SetRenderOpacity(1.0f);
					ApplyCenteredCanvasLayout(Button, FVector2D(-116.0f, 112.0f), FVector2D(220.0f, 44.0f));
					UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Reused forgot/reset button %s"), *Button->GetName());
				}
				else if (Name.Contains(TEXT("signup")) || Name.Contains(TEXT("register")) || Name.Contains(TEXT("createaccount")) ||
					Name.Contains(TEXT("create_account")))
				{
					Button->SetVisibility(ESlateVisibility::Visible);
					Button->SetRenderOpacity(1.0f);
					ApplyCenteredCanvasLayout(Button, FVector2D(116.0f, 112.0f), FVector2D(220.0f, 44.0f));
					UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Reused signup button %s"), *Button->GetName());
				}
				else if (Name.Contains(TEXT("backtologin")) || Name.Contains(TEXT("back_login")))
				{
					ApplyCenteredCanvasLayout(Button, FVector2D(-104.0f, 112.0f), FVector2D(198.0f, 40.0f));
				}
				else if (Name.Contains(TEXT("emailverification")))
				{
					ApplyCenteredCanvasLayout(Button, FVector2D(104.0f, 112.0f), FVector2D(198.0f, 40.0f));
				}
				else if (Name.Contains(TEXT("resetsubmit")) || Name.Contains(TEXT("signupsubmit")) || Name.Contains(TEXT("verifyemail")))
				{
					ApplyCenteredCanvasLayout(Button, FVector2D(0.0f, 56.0f), FVector2D(198.0f, 40.0f));
				}
				continue;
			}

			if (UTextBlock* TextBlock = Cast<UTextBlock>(Child))
			{
				if (Name == TEXT("txt_title"))
				{
					TextBlock->SetVisibility(ESlateVisibility::Collapsed);
					continue;
				}
				if (Name.Contains(TEXT("username")))
				{
					TextBlock->SetText(FText::FromString(TEXT("EMAIL")));
					UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Updated username label to EMAIL (%s)"), *TextBlock->GetName());
					TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.84f, 0.90f, 0.92f)));
					ApplyCenteredCanvasLayout(TextBlock, FVector2D(-118.0f, -126.0f), FVector2D(190.0f, 18.0f));
					continue;
				}
				if (Name == TEXT("txt_passwordlabel"))
				{
					TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.84f, 0.90f, 0.92f)));
					ApplyCenteredCanvasLayout(TextBlock, FVector2D(-118.0f, -52.0f), FVector2D(190.0f, 18.0f));
					continue;
				}
				if (Name == TEXT("txt_rememberme"))
				{
					TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.74f, 0.78f, 0.92f)));
					ApplyCenteredCanvasLayout(TextBlock, FVector2D(-110.0f, 16.0f), FVector2D(210.0f, 22.0f));
					continue;
				}
				if (Name.Contains(TEXT("forgot")) || Name.Contains(TEXT("help")) || Name.Contains(TEXT("createaccount")) ||
					Name.Contains(TEXT("signup")) || Name.Contains(TEXT("join")))
				{
					// Legacy text links are replaced by explicit action buttons.
					TextBlock->SetVisibility(ESlateVisibility::Collapsed);
					continue;
				}
				if (Name.Contains(TEXT("enter")) && Name.Contains(TEXT("username")))
				{
					TextBlock->SetText(FText::FromString(TEXT("Enter your email")));
					continue;
				}
				if (Name == TEXT("txt_loginbtn"))
				{
					TextBlock->SetVisibility(ESlateVisibility::Visible);
					TextBlock->SetRenderOpacity(1.0f);
					TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.97f, 0.97f, 0.98f, 1.0f)));
					continue;
				}

				if (Name.Contains(TEXT("title")) || Name.Contains(TEXT("header")))
				{
					TextBlock->SetColorAndOpacity(BodyText);
				}
				else
				{
					TextBlock->SetColorAndOpacity(SecondaryText);
				}
			}
		}

		// Force email hint text even if blueprint defaults are stale.
		if (UEditableTextBox* UsernameBox = Cast<UEditableTextBox>(RootWidget->WidgetTree->FindWidget(TEXT("TxtBox_Username"))))
		{
			UsernameBox->SetHintText(FText::FromString(TEXT("Enter your email")));
			UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Updated username hint to Enter your email"));
		}
		for (UWidget* Child : AllWidgets)
		{
			if (UEditableTextBox* TextBox = Cast<UEditableTextBox>(Child))
			{
				const FString Name = TextBox->GetName().ToLower();
				if (Name.Contains(TEXT("user")) || Name.Contains(TEXT("email")) || Name.Contains(TEXT("id")))
				{
					TextBox->SetHintText(FText::FromString(TEXT("Enter your email")));
				}
			}
		}

		UCanvasPanel* RootCanvas = nullptr;
		if (UButton* LoginButton = Cast<UButton>(RootWidget->WidgetTree->FindWidget(TEXT("Btn_Login"))))
		{
			RootCanvas = Cast<UCanvasPanel>(LoginButton->GetParent());
		}
		if (!RootCanvas)
		{
			RootCanvas = Cast<UCanvasPanel>(RootWidget->WidgetTree->RootWidget);
		}
		if (!RootCanvas)
		{
			for (UWidget* Child : AllWidgets)
			{
				if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(Child))
				{
					RootCanvas = Canvas;
					break;
				}
			}
		}

		const auto EnsureBottomButton = [&](const FName ButtonName, const TCHAR* Label, const FVector2D& Position, const bool bChangePassword)
		{
			if (!RootCanvas)
			{
				UE_LOG(LogTemp, Warning, TEXT("LoginPlayerController: RootCanvas missing, cannot create %s"), *ButtonName.ToString());
				return;
			}

			UButton* ActionButton = Cast<UButton>(RootWidget->WidgetTree->FindWidget(ButtonName));
			if (!ActionButton)
			{
				ActionButton = RootWidget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
				RootCanvas->AddChildToCanvas(ActionButton);
				UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Created action button %s"), *ButtonName.ToString());
			}

			if (!ActionButton)
			{
				return;
			}

			if (!Cast<UCanvasPanelSlot>(ActionButton->Slot))
			{
				ActionButton->RemoveFromParent();
				RootCanvas->AddChildToCanvas(ActionButton);
			}

			ActionButton->SetVisibility(ESlateVisibility::Visible);
			ActionButton->SetRenderOpacity(1.0f);
			ActionButton->SetStyle(BuildActionButtonStyle());
			ApplyCenteredCanvasLayout(ActionButton, Position, FVector2D(220.0f, 44.0f));
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ActionButton->Slot))
			{
				CanvasSlot->SetZOrder(25);
			}

			UTextBlock* LabelText = Cast<UTextBlock>(ActionButton->GetContent());
			if (!LabelText)
			{
				LabelText = RootWidget->WidgetTree->ConstructWidget<UTextBlock>(
					UTextBlock::StaticClass(),
					FName(*(ButtonName.ToString() + TEXT("_Label"))));
				ActionButton->SetContent(LabelText);
			}
			if (LabelText)
			{
				LabelText->SetText(FText::FromString(Label));
				LabelText->SetJustification(ETextJustify::Center);
				LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.97f, 1.0f, 1.0f)));
				FSlateFontInfo FontInfo = LabelText->GetFont();
				FontInfo.Size = 16;
				LabelText->SetFont(FontInfo);
			}

			ActionButton->OnClicked.Clear();
			if (OwnerController)
			{
				if (bChangePassword)
				{
					ActionButton->OnClicked.AddDynamic(OwnerController, &ALoginPlayerController::HandleChangePasswordClicked);
					UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Bound %s -> HandleChangePasswordClicked"), *ButtonName.ToString());
				}
				else
				{
					ActionButton->OnClicked.AddDynamic(OwnerController, &ALoginPlayerController::HandleCreateAccountClicked);
					UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Bound %s -> HandleCreateAccountClicked"), *ButtonName.ToString());
				}
			}
		};

		EnsureBottomButton(TEXT("Btn_ChangePassword"), TEXT("비밀번호 찾기"), FVector2D(-116.0f, 112.0f), true);
		EnsureBottomButton(TEXT("Btn_CreateAccount"), TEXT("CREATE ACCOUNT"), FVector2D(116.0f, 112.0f), false);

		if (RootCanvas && OwnerController)
		{
			UTextBlock* SubviewTitle = Cast<UTextBlock>(RootWidget->WidgetTree->FindWidget(TEXT("Txt_SubviewTitle")));
			if (!SubviewTitle)
			{
				SubviewTitle = RootWidget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_SubviewTitle"));
				RootCanvas->AddChildToCanvas(SubviewTitle);
			}
			if (SubviewTitle)
			{
				SubviewTitle->SetText(FText::FromString(TEXT("")));
				SubviewTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.97f, 1.0f, 1.0f)));
				SubviewTitle->SetVisibility(ESlateVisibility::Collapsed);
				SubviewTitle->SetIsEnabled(false);
				if (UCanvasPanelSlot* TitleSlot = Cast<UCanvasPanelSlot>(SubviewTitle->Slot))
				{
					TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
					TitleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
					TitleSlot->SetPosition(FVector2D(0.0f, -162.0f));
					TitleSlot->SetSize(FVector2D(360.0f, 36.0f));
					TitleSlot->SetZOrder(40);
				}
			}

			const auto EnsureAuxButton =
				[&](const FName ButtonName, const TCHAR* Label, const FVector2D& Position, const FName HandlerName)
			{
				UButton* AuxButton = Cast<UButton>(RootWidget->WidgetTree->FindWidget(ButtonName));
				if (!AuxButton)
				{
					AuxButton = RootWidget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
					RootCanvas->AddChildToCanvas(AuxButton);
				}
				if (!AuxButton)
				{
					return;
				}

				if (!Cast<UCanvasPanelSlot>(AuxButton->Slot))
				{
					AuxButton->RemoveFromParent();
					RootCanvas->AddChildToCanvas(AuxButton);
				}

				AuxButton->SetStyle(BuildActionButtonStyle());
				AuxButton->SetVisibility(ESlateVisibility::Collapsed);
				if (UCanvasPanelSlot* BtnSlot = Cast<UCanvasPanelSlot>(AuxButton->Slot))
				{
					BtnSlot->SetAnchors(FAnchors(0.5f, 0.5f));
					BtnSlot->SetAlignment(FVector2D(0.5f, 0.5f));
					BtnSlot->SetPosition(FVector2D(Position.X, Position.Y + FormCenterYOffset));
					BtnSlot->SetSize(FVector2D(198.0f, 40.0f));
					BtnSlot->SetZOrder(40);
				}

				UTextBlock* LabelText = Cast<UTextBlock>(AuxButton->GetContent());
				if (!LabelText)
				{
					LabelText = RootWidget->WidgetTree->ConstructWidget<UTextBlock>(
						UTextBlock::StaticClass(),
						FName(*(ButtonName.ToString() + TEXT("_Label"))));
					AuxButton->SetContent(LabelText);
				}
				if (LabelText)
				{
					LabelText->SetText(FText::FromString(Label));
					LabelText->SetJustification(ETextJustify::Center);
					LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.97f, 1.0f, 1.0f)));
				}

				AuxButton->OnClicked.Clear();
				FScriptDelegate Delegate;
				Delegate.BindUFunction(OwnerController, HandlerName);
				AuxButton->OnClicked.Add(Delegate);
			};

			EnsureAuxButton(TEXT("Btn_BackToLogin"), TEXT("BACK"), FVector2D(-104.0f, 112.0f), TEXT("HandleBackToLoginClicked"));
			EnsureAuxButton(TEXT("Btn_EmailVerification"), TEXT("VERIFY EMAIL"), FVector2D(104.0f, 112.0f), TEXT("HandleOpenEmailVerificationClicked"));
			EnsureAuxButton(TEXT("Btn_ResetSubmit"), TEXT("CONFIRM CHANGE"), FVector2D(0.0f, 56.0f), TEXT("HandleResetSubmitClicked"));
			EnsureAuxButton(TEXT("Btn_SignupSubmit"), TEXT("CREATE ACCOUNT"), FVector2D(0.0f, 56.0f), TEXT("HandleSignupSubmitClicked"));
			EnsureAuxButton(TEXT("Btn_VerifyEmail"), TEXT("VERIFY EMAIL"), FVector2D(0.0f, 56.0f), TEXT("HandleVerifyEmailClicked"));
		}
	}
}

ALoginPlayerController::ALoginPlayerController()
{
	// Login form layer.
	static ConstructorHelpers::FClassFinder<UUserWidget> LegacyLoginFinder(TEXT("/Game/Widgets/WBP_Login"));
	if (LegacyLoginFinder.Succeeded())
	{
		AuthWidgetClass = LegacyLoginFinder.Class;
	}

	// Background layer.
	static ConstructorHelpers::FClassFinder<UUserWidget> AuthMasterFinder(TEXT("/Game/Widgets/WBP_AuthMaster"));
	if (AuthMasterFinder.Succeeded())
	{
		BackgroundWidgetClass = AuthMasterFinder.Class;
	}
}

void ALoginPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Primary auth flow uses dedicated Slate panels:
	// Login, Forgot Password, Sign Up, Email Verification (separate forms).
	if (GEngine && GEngine->GameViewport)
	{
		SlateAuthWidget = SNew(SAuthMasterWidget);
		GEngine->GameViewport->AddViewportWidgetContent(SlateAuthWidget.ToSharedRef(), 10);

		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);

		FInputModeUIOnly InputModeData;
		InputModeData.SetWidgetToFocus(SlateAuthWidget);
		SetInputMode(InputModeData);
		bShowMouseCursor = true;
		UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Using Slate auth master widget"));
		return;
	}

	if (!AuthWidgetClass)
	{
		UClass* LoadedClass = StaticLoadClass(
			UUserWidget::StaticClass(), nullptr, TEXT("/Game/Widgets/WBP_Login.WBP_Login_C"));
		AuthWidgetClass = LoadedClass;
	}

	if (!BackgroundWidgetClass)
	{
		UClass* LoadedBackgroundClass = StaticLoadClass(
			UUserWidget::StaticClass(), nullptr, TEXT("/Game/Widgets/WBP_AuthMaster.WBP_AuthMaster_C"));
		BackgroundWidgetClass = LoadedBackgroundClass;
	}

	if (BackgroundWidgetClass)
	{
		BackgroundWidget = CreateWidget<UUserWidget>(this, BackgroundWidgetClass);
		if (BackgroundWidget)
		{
			BackgroundWidget->AddToViewport(0);
			UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Added background widget class %s"),
				*BackgroundWidgetClass->GetName());
		}
	}

	if (!AuthWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("LoginPlayerController: AuthWidgetClass not found (WBP_Login)"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Using auth widget class %s"), *AuthWidgetClass->GetName());
	AuthWidget = CreateWidget<UUserWidget>(this, AuthWidgetClass);
	if (AuthWidget)
	{
		ApplyDarkMmorpgStyle(AuthWidget, this);
		AuthWidget->AddToViewport(10);

		FInputModeUIOnly InputModeData;
		SetInputMode(InputModeData);
		bShowMouseCursor = true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("LoginPlayerController: Failed to create auth widget from class %s"),
			*AuthWidgetClass->GetName());
	}
}

void ALoginPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SlateAuthWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(SlateAuthWidget.ToSharedRef());
		SlateAuthWidget.Reset();
	}

	if (AuthWidget)
	{
		AuthWidget->RemoveFromParent();
		AuthWidget = nullptr;
	}
	if (BackgroundWidget)
	{
		BackgroundWidget->RemoveFromParent();
		BackgroundWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ALoginPlayerController::HandleChangePasswordClicked()
{
	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: HandleChangePasswordClicked invoked"));
	if (!AuthWidget)
	{
		return;
	}

	const bool bExecuted = ExecuteFirstAvailableAuthFunction(
		{
			TEXT("OnResetPasswordClicked"),
			TEXT("HandleResetPasswordClicked"),
			TEXT("SwitchToResetPassword"),
			TEXT("OpenForgotPassword"),
			TEXT("OnForgotPasswordClicked"),
		});
	if (bExecuted)
	{
		return;
	}

	SetNamedWidgetsVisibility(
		AuthWidget,
		{
			TEXT("txtbox_username"),
			TEXT("txtbox_password"),
			TEXT("txt_usernamelabel"),
			TEXT("txt_passwordlabel"),
			TEXT("txt_rememberme"),
			TEXT("btn_login"),
			TEXT("txt_loginbtn"),
			TEXT("txt_forgotpassword"),
			TEXT("txt_help"),
			TEXT("txt_createaccount"),
			TEXT("btn_changepassword"),
			TEXT("btn_createaccount")
		},
		ESlateVisibility::Collapsed);
	SetNamedWidgetsVisibility(AuthWidget, {TEXT("txt_subviewtitle"), TEXT("btn_backtologin"), TEXT("btn_emailverification"), TEXT("btn_resetsubmit")}, ESlateVisibility::Visible);
	SetNamedWidgetsVisibility(AuthWidget, {TEXT("btn_signupsubmit"), TEXT("btn_verifyemail")}, ESlateVisibility::Collapsed);
	SetEditableTextBoxesVisibilityByName(
		AuthWidget,
		{
			TEXT("resetemail"),
			TEXT("verifycode"),
			TEXT("newpassword"),
			TEXT("confirmpassword"),
			TEXT("resetpassword"),
			TEXT("verify"),
			TEXT("code"),
			TEXT("email"),
			TEXT("password"),
			TEXT("txtbox")
		},
		ESlateVisibility::Visible);
	SetEditableTextBoxesVisibilityByName(
		AuthWidget,
		{
			TEXT("signupemail"),
			TEXT("signuppassword"),
			TEXT("registeremail"),
			TEXT("registerpassword")
		},
		ESlateVisibility::Collapsed);
	EnsureFallbackEditableTextBoxesVisible(AuthWidget);

	if (UTextBlock* Title = Cast<UTextBlock>(AuthWidget->WidgetTree->FindWidget(TEXT("Txt_SubviewTitle"))))
	{
		Title->SetText(FText::FromString(TEXT("비밀번호 찾기")));
	}
	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Fallback change-password subview opened"));
}

void ALoginPlayerController::HandleCreateAccountClicked()
{
	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: HandleCreateAccountClicked invoked"));
	if (!AuthWidget)
	{
		return;
	}
	const bool bExecuted = ExecuteFirstAvailableAuthFunction(
		{
			TEXT("OnSignUpClicked"),
			TEXT("HandleSignUpClicked"),
			TEXT("SwitchToSignUp"),
			TEXT("OpenSignUp"),
			TEXT("OnCreateAccountClicked"),
			TEXT("OnRegisterClicked"),
		});
	if (bExecuted)
	{
		return;
	}

	SetNamedWidgetsVisibility(
		AuthWidget,
		{
			TEXT("txtbox_username"),
			TEXT("txtbox_password"),
			TEXT("txt_usernamelabel"),
			TEXT("txt_passwordlabel"),
			TEXT("txt_rememberme"),
			TEXT("btn_login"),
			TEXT("txt_loginbtn"),
			TEXT("txt_forgotpassword"),
			TEXT("txt_help"),
			TEXT("txt_createaccount"),
			TEXT("btn_changepassword"),
			TEXT("btn_createaccount")
		},
		ESlateVisibility::Collapsed);
	SetNamedWidgetsVisibility(AuthWidget, {TEXT("txt_subviewtitle"), TEXT("btn_backtologin"), TEXT("btn_emailverification"), TEXT("btn_signupsubmit")}, ESlateVisibility::Visible);
	SetNamedWidgetsVisibility(AuthWidget, {TEXT("btn_resetsubmit"), TEXT("btn_verifyemail")}, ESlateVisibility::Collapsed);
	SetEditableTextBoxesVisibilityByName(
		AuthWidget,
		{
			TEXT("signupemail"),
			TEXT("signuppassword"),
			TEXT("registeremail"),
			TEXT("registerpassword"),
			TEXT("confirmpassword"),
			TEXT("signup"),
			TEXT("register"),
			TEXT("confirm"),
			TEXT("email"),
			TEXT("password"),
			TEXT("txtbox")
		},
		ESlateVisibility::Visible);
	SetEditableTextBoxesVisibilityByName(
		AuthWidget,
		{
			TEXT("resetemail"),
			TEXT("verifycode"),
			TEXT("newpassword"),
			TEXT("resetpassword")
		},
		ESlateVisibility::Collapsed);
	EnsureFallbackEditableTextBoxesVisible(AuthWidget);

	if (UTextBlock* Title = Cast<UTextBlock>(AuthWidget->WidgetTree->FindWidget(TEXT("Txt_SubviewTitle"))))
	{
		Title->SetText(FText::FromString(TEXT("CREATE ACCOUNT")));
	}
	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: Fallback create-account subview opened"));
}

void ALoginPlayerController::HandleBackToLoginClicked()
{
	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: HandleBackToLoginClicked invoked"));
	if (!AuthWidget)
	{
		return;
	}

	SetNamedWidgetsVisibility(
		AuthWidget,
		{
			TEXT("txtbox_username"),
			TEXT("txtbox_password"),
			TEXT("txt_usernamelabel"),
			TEXT("txt_passwordlabel"),
			TEXT("txt_rememberme"),
			TEXT("btn_login"),
			TEXT("txt_loginbtn"),
			TEXT("txt_forgotpassword"),
			TEXT("txt_help"),
			TEXT("txt_createaccount"),
			TEXT("btn_changepassword"),
			TEXT("btn_createaccount")
		},
		ESlateVisibility::Visible);
	SetNamedWidgetsVisibility(
		AuthWidget,
		{
			TEXT("txt_forgotpassword"),
			TEXT("txt_help"),
			TEXT("txt_createaccount")
		},
		ESlateVisibility::Collapsed);
	SetNamedWidgetsVisibility(
		AuthWidget,
		{
			TEXT("txt_subviewtitle"),
			TEXT("btn_backtologin"),
			TEXT("btn_emailverification"),
			TEXT("btn_resetsubmit"),
			TEXT("btn_signupsubmit"),
			TEXT("btn_verifyemail")
		},
		ESlateVisibility::Collapsed);
}

void ALoginPlayerController::HandleOpenEmailVerificationClicked()
{
	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: HandleOpenEmailVerificationClicked invoked"));
	if (!AuthWidget)
	{
		return;
	}

	SetNamedWidgetsVisibility(AuthWidget, {TEXT("txt_subviewtitle"), TEXT("btn_backtologin"), TEXT("btn_verifyemail")}, ESlateVisibility::Visible);
	SetNamedWidgetsVisibility(AuthWidget, {TEXT("btn_emailverification"), TEXT("btn_resetsubmit"), TEXT("btn_signupsubmit")}, ESlateVisibility::Collapsed);
	SetEditableTextBoxesVisibilityByName(
		AuthWidget,
		{
			TEXT("verifycode"),
			TEXT("emailverification"),
			TEXT("verify"),
			TEXT("code"),
			TEXT("email"),
			TEXT("txtbox")
		},
		ESlateVisibility::Visible);
	SetEditableTextBoxesVisibilityByName(
		AuthWidget,
		{
			TEXT("signupemail"),
			TEXT("signuppassword"),
			TEXT("registeremail"),
			TEXT("registerpassword"),
			TEXT("resetemail"),
			TEXT("newpassword"),
			TEXT("confirmpassword")
		},
		ESlateVisibility::Collapsed);
	EnsureFallbackEditableTextBoxesVisible(AuthWidget);
	if (UTextBlock* Title = Cast<UTextBlock>(AuthWidget->WidgetTree->FindWidget(TEXT("Txt_SubviewTitle"))))
	{
		Title->SetText(FText::FromString(TEXT("EMAIL VERIFICATION")));
	}
}

void ALoginPlayerController::HandleResetSubmitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: HandleResetSubmitClicked invoked"));
	const bool bExecuted = ExecuteFirstAvailableAuthFunction(
		{
			TEXT("OnResetSubmitClicked"),
			TEXT("SubmitResetPassword"),
			TEXT("HandleResetPasswordSubmit"),
			TEXT("OnConfirmResetPasswordClicked"),
		});
	if (bExecuted)
	{
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Password change submit clicked."));
	}
}

void ALoginPlayerController::HandleSignupSubmitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: HandleSignupSubmitClicked invoked"));
	const bool bExecuted = ExecuteFirstAvailableAuthFunction(
		{
			TEXT("OnSignUpSubmitClicked"),
			TEXT("SubmitSignUp"),
			TEXT("HandleCreateAccountSubmit"),
			TEXT("OnRegisterSubmitClicked"),
		});
	if (bExecuted)
	{
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Create account submit clicked."));
	}
}

void ALoginPlayerController::HandleVerifyEmailClicked()
{
	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: HandleVerifyEmailClicked invoked"));
	const bool bExecuted = ExecuteFirstAvailableAuthFunction(
		{
			TEXT("OnVerifyEmailClicked"),
			TEXT("SubmitVerifyCode"),
			TEXT("HandleVerifyCodeSubmit"),
			TEXT("OnEmailVerifySubmitClicked"),
		});
	if (bExecuted)
	{
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Email verification clicked."));
	}
}

bool ALoginPlayerController::ExecuteFirstAvailableAuthFunction(const TArray<FName>& CandidateFunctions) const
{
	if (!AuthWidget)
	{
		return false;
	}

	for (const FName& FunctionName : CandidateFunctions)
	{
		UFunction* Function = AuthWidget->FindFunction(FunctionName);
		if (!Function)
		{
			continue;
		}

		AuthWidget->ProcessEvent(Function, nullptr);
		UE_LOG(
			LogTemp,
			Log,
			TEXT("LoginPlayerController: Executed auth widget function %s"),
			*FunctionName.ToString());
		return true;
	}

	UE_LOG(LogTemp, Log, TEXT("LoginPlayerController: No legacy auth function found (keeping current view)"));
	return false;
}
