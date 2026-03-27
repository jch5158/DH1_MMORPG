#include "AuthMasterWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "LoginWidget.h"
#include "SignUpWidget.h"
#include "EmailVerificationWidget.h"
#include "ResetPasswordWidget.h"
#include "RealmSelectWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

void UAuthMasterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoginWidget)
	{
		LoginWidget->OnGoToSignUp.AddUObject(this, &UAuthMasterWidget::HandleGoToSignUp);
		LoginWidget->OnLoginSuccess.AddUObject(this, &UAuthMasterWidget::HandleLoginSuccess);
		LoginWidget->OnGoToResetPassword.AddUObject(this, &UAuthMasterWidget::HandleGoToResetPassword);
	}

	if (SignUpWidget)
	{
		SignUpWidget->OnGoToLogin.AddUObject(this, &UAuthMasterWidget::HandleGoToLogin);
		SignUpWidget->OnGoToEmailVerification.AddUObject(this, &UAuthMasterWidget::HandleGoToEmailVerification);
	}

	if (EmailVerificationWidget)
	{
		EmailVerificationWidget->OnGoToLogin.AddUObject(this, &UAuthMasterWidget::HandleGoToLogin);
	}

	if (ResetPasswordWidget)
	{
		ResetPasswordWidget->OnGoToLogin.AddUObject(this, &UAuthMasterWidget::HandleGoToLogin);
	}
}

void UAuthMasterWidget::HandleGoToLogin(const FString& StatusText, const FString& Email) const
{
	SwitchWidget(EAuthWidgetType::Login, StatusText, Email);
}

void UAuthMasterWidget::HandleGoToEmailVerification(const FString& StatusText, const FString& Email) const
{
	SwitchWidget(EAuthWidgetType::EmailVerification, StatusText, Email);
}

void UAuthMasterWidget::HandleGoToSignUp() const
{
	SwitchWidget(EAuthWidgetType::SignUp, "", "");
}

void UAuthMasterWidget::HandleLoginSuccess()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>();
	if (NetSubsystem == nullptr)
	{
		return;
	}

	// 렐름 선택 결과 수신 시 LobbyLevel 진입
	NetSubsystem->OnRealmSelectResult.AddLambda([this, GameInstance](const int32 Result)
		{
			if (Result == 0)
			{
				if (RealmSelectWidgetRef.IsValid() && GEngine && GEngine->GameViewport)
				{
					GEngine->GameViewport->RemoveViewportWidgetContent(RealmSelectWidgetRef.ToSharedRef());
					RealmSelectWidgetRef.Reset();
				}

				UGameplayStatics::OpenLevel(GameInstance, LobbyLevelName);
			}
		});

	// 렐름 목록 수신 시 RealmSelectWidget 표시
	NetSubsystem->OnRealmListReceived.AddLambda([this](const TArray<FRealmServerInfo>& RealmList)
		{
			SetVisibility(ESlateVisibility::Collapsed);

			TSharedRef<SRealmSelectWidget> SelectWidget = SNew(SRealmSelectWidget).RealmList(RealmList);
			RealmSelectWidgetRef = SelectWidget;
			GEngine->GameViewport->AddViewportWidgetContent(SelectWidget);
		});

	// 렐름 목록 요청
	NetSubsystem->RequestRealmList();
}

void UAuthMasterWidget::HandleGoToResetPassword() const
{
	SwitchWidget(EAuthWidgetType::ResetPassword, "", "");
}

void UAuthMasterWidget::SwitchWidget(const EAuthWidgetType WidgetType, const FString& StatusText, const FString& Email) const
{
	if (!MainSwitcher)
	{
		return;
	}

	switch (WidgetType)
	{
	case EAuthWidgetType::Login:

		if (LoginWidget)
		{
			LoginWidget->ResetLoginWidget(StatusText, Email);
			MainSwitcher->SetActiveWidgetIndex(0);
		}

		break;
	case EAuthWidgetType::SignUp:

		if (SignUpWidget)
		{
			SignUpWidget->ResetSignUpWidget();
			MainSwitcher->SetActiveWidgetIndex(1);
		}

		break;
	case EAuthWidgetType::EmailVerification:

		if (EmailVerificationWidget)
		{
			EmailVerificationWidget->ResetVerificationWidget(StatusText, Email);
			MainSwitcher->SetActiveWidgetIndex(2);
		}

		break;
	case EAuthWidgetType::ResetPassword:

		if (ResetPasswordWidget)
		{
			ResetPasswordWidget->ResetResetPasswordWidget(StatusText, Email);
			MainSwitcher->SetActiveWidgetIndex(3);
		}

		break;
	default:
		UE_LOG(LogTemp, Error, TEXT("UAuthMasterWidget::SwitchWidget - 처리되지 않은 ELoginWidgetType 값입니다."));
		break;
	}
}
