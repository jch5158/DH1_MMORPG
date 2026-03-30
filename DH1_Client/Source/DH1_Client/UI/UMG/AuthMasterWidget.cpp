#include "UI/UMG/AuthMasterWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "UI/AuthWidgetStyle.h"

namespace
{
	UImage* ResolveBackgroundImage(UAuthMasterWidget* Widget)
	{
		if (!Widget)
		{
			return nullptr;
		}

		if (UImage* Exact = Cast<UImage>(Widget->GetWidgetFromName(TEXT("BgImage"))))
		{
			return Exact;
		}

		if (!Widget->WidgetTree)
		{
			return nullptr;
		}

		TArray<UWidget*> AllWidgets;
		Widget->WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* Child : AllWidgets)
		{
			UImage* Image = Cast<UImage>(Child);
			if (!Image)
			{
				continue;
			}

			const FString Name = Image->GetName().ToLower();
			if (Name.Contains(TEXT("bg")) || Name.Contains(TEXT("background")) || Name.Contains(TEXT("loginbg")))
			{
				return Image;
			}
		}

		return nullptr;
	}

	void ApplyFullScreenBackgroundLayout(UImage* BgImg)
	{
		if (!BgImg)
		{
			return;
		}

		BgImg->SetDesiredSizeOverride(FVector2D(4096.0f, 4096.0f));
		BgImg->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UPanelWidget* Parent = BgImg->GetParent())
		{
			const int32 ChildIndex = Parent->GetChildIndex(BgImg);
			if (ChildIndex > 0)
			{
				Parent->RemoveChild(BgImg);
				Parent->InsertChildAt(0, BgImg);
			}
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BgImg->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CanvasSlot->SetOffsets(FMargin(0.0f));
			CanvasSlot->SetAlignment(FVector2D::ZeroVector);
			CanvasSlot->SetAutoSize(false);
			CanvasSlot->SetZOrder(-1000);
			UE_LOG(LogTemp, Log, TEXT("AuthMasterWidget: BgImage full-screen via CanvasPanelSlot"));
			return;
		}

		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(BgImg->Slot))
		{
			OverlaySlot->SetPadding(FMargin(0.0f));
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			UE_LOG(LogTemp, Log, TEXT("AuthMasterWidget: BgImage full-screen via OverlaySlot"));
			return;
		}

		if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(BgImg->Slot))
		{
			BorderSlot->SetPadding(FMargin(0.0f));
			BorderSlot->SetHorizontalAlignment(HAlign_Fill);
			BorderSlot->SetVerticalAlignment(VAlign_Fill);
			UE_LOG(LogTemp, Log, TEXT("AuthMasterWidget: BgImage full-screen via BorderSlot"));
			return;
		}

		if (UPanelSlot* PanelSlot = Cast<UPanelSlot>(BgImg->Slot))
		{
			UE_LOG(LogTemp, Warning, TEXT("AuthMasterWidget: BgImage slot type is %s (fallback sizing only)"),
				*PanelSlot->GetClass()->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AuthMasterWidget: BgImage has no panel slot (fallback sizing only)"));
		}
	}

	void ApplyInputWidgetStyle(UAuthMasterWidget* Widget)
	{
		if (!Widget || !Widget->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> AllWidgets;
		Widget->WidgetTree->GetAllWidgets(AllWidgets);

		for (UWidget* Child : AllWidgets)
		{
			UEditableTextBox* TextBox = Cast<UEditableTextBox>(Child);
			if (!TextBox)
			{
				continue;
			}

			FEditableTextBoxStyle InputStyle = AuthStyle::InputStyle();
			const FLinearColor TranslucentBlack(0.02f, 0.02f, 0.02f, 0.68f);
			const FLinearColor HoverBlack(0.05f, 0.05f, 0.05f, 0.78f);

			InputStyle.BackgroundImageNormal.TintColor = FSlateColor(TranslucentBlack);
			InputStyle.BackgroundImageHovered.TintColor = FSlateColor(HoverBlack);
			InputStyle.BackgroundImageFocused.TintColor = FSlateColor(HoverBlack);
			InputStyle.BackgroundImageReadOnly.TintColor = FSlateColor(TranslucentBlack);

			TextBox->SetWidgetStyle(InputStyle);
		}
	}

	void ApplyDarkMmorpgWidgetStyle(UAuthMasterWidget* Widget)
	{
		if (!Widget || !Widget->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> AllWidgets;
		Widget->WidgetTree->GetAllWidgets(AllWidgets);

		const FLinearColor PanelDark(0.01f, 0.01f, 0.01f, 0.62f);
		const FLinearColor PanelDarkSoft(0.02f, 0.02f, 0.02f, 0.46f);
		const FLinearColor ButtonDark(0.03f, 0.03f, 0.03f, 0.78f);
		const FLinearColor ButtonHover(0.08f, 0.08f, 0.08f, 0.86f);
		const FSlateColor BodyText(FLinearColor(0.90f, 0.90f, 0.90f, 1.0f));
		const FSlateColor SecondaryText(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));

		for (UWidget* Child : AllWidgets)
		{
			const FString Name = Child->GetName().ToLower();

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

			if (UButton* Button = Cast<UButton>(Child))
			{
				FButtonStyle Style = Button->GetStyle();
				Style.Normal.TintColor = FSlateColor(ButtonDark);
				Style.Hovered.TintColor = FSlateColor(ButtonHover);
				Style.Pressed.TintColor = FSlateColor(PanelDark);
				Style.Disabled.TintColor = FSlateColor(PanelDarkSoft);
				Button->SetStyle(Style);
				continue;
			}

			if (UTextBlock* TextBlock = Cast<UTextBlock>(Child))
			{
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
	}

	void ApplyDevhunLogoTransform(UAuthMasterWidget* Widget)
	{
		if (!Widget || !Widget->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> AllWidgets;
		Widget->WidgetTree->GetAllWidgets(AllWidgets);

		for (UWidget* Child : AllWidgets)
		{
			UImage* Image = Cast<UImage>(Child);
			if (!Image)
			{
				continue;
			}

			const FString WidgetName = Image->GetName().ToLower();
			const bool bIsDevhun = WidgetName.Contains(TEXT("devhun"));
			const bool bIsUnreal = WidgetName.Contains(TEXT("unreal"));
			if (!bIsDevhun && !bIsUnreal)
			{
				continue;
			}

			Image->SetRenderScale(FVector2D(0.78f, 0.78f));

			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Image->Slot))
			{
				const FVector2D CurrentPos = CanvasSlot->GetPosition();
				if (bIsDevhun)
				{
					CanvasSlot->SetPosition(FVector2D(FMath::Max(24.0f, CurrentPos.X - 60.0f), CurrentPos.Y));
				}
				else if (bIsUnreal)
				{
					CanvasSlot->SetPosition(FVector2D(CurrentPos.X, CurrentPos.Y));
				}
			}
		}
	}
}

void UAuthMasterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UImage* BgImg = ResolveBackgroundImage(this))
	{
		BgImg->SetBrush(*AuthStyle::GetLoginBgBrush());
		BgImg->SetColorAndOpacity(FLinearColor::White);
		ApplyFullScreenBackgroundLayout(BgImg);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AuthMasterWidget: No background image widget found; background image cannot be applied"));
	}

	ApplyInputWidgetStyle(this);
	ApplyDarkMmorpgWidgetStyle(this);
	ApplyDevhunLogoTransform(this);

	if (UClientNetSubsystem* Net = GetNetSubsystem())
	{
		Net->OnGatewayLoginResult.AddUObject(this, &UAuthMasterWidget::HandleGatewayLoginResult);
		Net->OnHttpLoginError.AddUObject(this, &UAuthMasterWidget::HandleHttpLoginError);
		Net->OnEmailVerificationRequired.AddUObject(this, &UAuthMasterWidget::HandleEmailVerificationRequired);
		Net->OnRealmListReceived.AddUObject(this, &UAuthMasterWidget::HandleRealmListReceived);
		Net->OnRealmSelectResult.AddUObject(this, &UAuthMasterWidget::HandleRealmSelectResult);
	}
}

void UAuthMasterWidget::NativeDestruct()
{
	if (UClientNetSubsystem* Net = GetNetSubsystem())
	{
		Net->OnGatewayLoginResult.RemoveAll(this);
		Net->OnHttpLoginError.RemoveAll(this);
		Net->OnEmailVerificationRequired.RemoveAll(this);
		Net->OnRealmListReceived.RemoveAll(this);
		Net->OnRealmSelectResult.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UAuthMasterWidget::RequestLogin(const FString& Email, const FString& Password)
{
	if (UClientNetSubsystem* Net = GetNetSubsystem())
	{
		Net->RequestLogin(Email, Password);
	}
}

void UAuthMasterWidget::RequestRealmList()
{
	if (UClientNetSubsystem* Net = GetNetSubsystem())
	{
		Net->RequestRealmList();
	}
}

void UAuthMasterWidget::RequestRealmSelect(int32 RealmId)
{
	if (UClientNetSubsystem* Net = GetNetSubsystem())
	{
		Net->RequestRealmSelect(RealmId);
	}
}

void UAuthMasterWidget::OpenLobbyLevel()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Levels/L_Lobby")), true,
		TEXT("?Game=/Script/DH1_Client.LobbyGameMode"));
}

void UAuthMasterWidget::HandleGatewayLoginResult(int32 Result)
{
	OnGatewayLoginResult(Result);
}

void UAuthMasterWidget::HandleHttpLoginError(int32 StatusCode, const FString& Message)
{
	OnHttpLoginError(StatusCode, Message);
}

void UAuthMasterWidget::HandleEmailVerificationRequired(const FString& Message, const FString& Email)
{
	OnEmailVerificationRequired(Message, Email);
}

void UAuthMasterWidget::HandleRealmListReceived(const TArray<FRealmServerInfo>& RealmList)
{
	OnRealmListReady(RealmList);
}

void UAuthMasterWidget::HandleRealmSelectResult(int32 Result)
{
	OnRealmSelectDone(Result);
}

UClientNetSubsystem* UAuthMasterWidget::GetNetSubsystem() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UClientNetSubsystem>() : nullptr;
}
