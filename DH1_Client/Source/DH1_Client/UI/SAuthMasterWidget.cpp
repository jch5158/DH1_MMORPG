#include "SAuthMasterWidget.h"
#include "SLoginPanel.h"
#include "SSignUpPanel.h"
#include "SEmailVerificationPanel.h"
#include "SResetPasswordPanel.h"
#include "RealmSelectWidget.h"
#include "AuthWidgetStyle.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SBox.h"

SAuthMasterWidget::~SAuthMasterWidget()
{
	if (UClientNetSubsystem* Net = GetNetSubsystem())
	{
		Net->OnGatewayLoginResult.RemoveAll(this);
		Net->OnHttpLoginError.RemoveAll(this);
		Net->OnEmailVerificationRequired.RemoveAll(this);
		Net->OnRealmListReceived.RemoveAll(this);
		Net->OnRealmSelectResult.RemoveAll(this);
	}
}

void SAuthMasterWidget::Construct(const FArguments& InArgs)
{
	// Keep auth card slightly below DH1 logo with a consistent offset.
	constexpr float AuthTopPadding = 282.0f;

	SAssignNew(PanelSwitcher, SWidgetSwitcher)

			// 0: Login
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(LoginPanel, SLoginPanel)
				.OnGoToSignUp_Lambda([this]()
				{
					SwitchToSignUp();
				})
				.OnGoToResetPassword_Lambda([this]()
				{
					SwitchToResetPassword();
				})
				.OnLoginSuccess_Lambda([this]()
				{
					HandleLoginSuccess();
				})
				.OnGoToEmailVerification_Lambda([this](const FString& StatusText, const FString& Email, bool bSkipAutoSend)
				{
					SwitchToEmailVerification(StatusText, Email, bSkipAutoSend);
				})
			]

			// 1: SignUp
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(SignUpPanel, SSignUpPanel)
				.OnGoToLogin_Lambda([this](const FString& StatusText, const FString& Email)
				{
					SwitchToLogin(StatusText, Email);
				})
				.OnGoToEmailVerification_Lambda([this](const FString& StatusText, const FString& Email, bool bSkipAutoSend)
				{
					SwitchToEmailVerification(StatusText, Email, bSkipAutoSend);
				})
			]

			// 2: EmailVerification
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(EmailVerificationPanel, SEmailVerificationPanel)
				.OnGoToLogin_Lambda([this](const FString& StatusText, const FString& Email)
				{
					SwitchToLogin(StatusText, Email);
				})
			]

			// 3: ResetPassword
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(ResetPasswordPanel, SResetPasswordPanel)
				.OnGoToLogin_Lambda([this](const FString& StatusText, const FString& Email)
				{
					SwitchToLogin(StatusText, Email);
				})
			]
		;

	ChildSlot
	[
		AuthStyle::MakeVampireBackground(
			PanelSwitcher.ToSharedRef(),
			HAlign_Center,
			VAlign_Top,
			FMargin(0.0f, AuthTopPadding, 0.0f, 0.0f))
	];

	// Bind ClientNetSubsystem delegates
	if (UClientNetSubsystem* Net = GetNetSubsystem())
	{
		Net->OnGatewayLoginResult.AddRaw(this, &SAuthMasterWidget::HandleGatewayLoginResult);
		Net->OnHttpLoginError.AddRaw(this, &SAuthMasterWidget::HandleHttpLoginError);
		Net->OnEmailVerificationRequired.AddRaw(this, &SAuthMasterWidget::HandleEmailVerificationRequired);
	}

	// Start on Login panel
	if (PanelSwitcher.IsValid())
	{
		PanelSwitcher->SetActiveWidgetIndex(0);
	}
}

void SAuthMasterWidget::SwitchToLogin(const FString& StatusText, const FString& Email)
{
	if (LoginPanel.IsValid())
	{
		LoginPanel->ResetPanel(StatusText, Email);
	}

	if (PanelSwitcher.IsValid())
	{
		PanelSwitcher->SetActiveWidgetIndex(0);
	}
}

void SAuthMasterWidget::SwitchToSignUp()
{
	if (SignUpPanel.IsValid())
	{
		SignUpPanel->ResetPanel();
	}

	if (PanelSwitcher.IsValid())
	{
		PanelSwitcher->SetActiveWidgetIndex(1);
	}
}

void SAuthMasterWidget::SwitchToEmailVerification(const FString& StatusText, const FString& Email, const bool bSkipAutoSendVerifyCode)
{
	if (EmailVerificationPanel.IsValid())
	{
		EmailVerificationPanel->ResetPanel(StatusText, Email, bSkipAutoSendVerifyCode);
	}

	if (PanelSwitcher.IsValid())
	{
		PanelSwitcher->SetActiveWidgetIndex(2);
	}
}

void SAuthMasterWidget::SwitchToResetPassword()
{
	if (ResetPasswordPanel.IsValid())
	{
		ResetPasswordPanel->ResetPanel(TEXT(""), TEXT(""));
	}

	if (PanelSwitcher.IsValid())
	{
		PanelSwitcher->SetActiveWidgetIndex(3);
	}
}

void SAuthMasterWidget::HandleLoginSuccess()
{
	UClientNetSubsystem* Net = GetNetSubsystem();
	if (Net == nullptr)
	{
		return;
	}

	Net->OnRealmSelectResult.RemoveAll(this);
	Net->OnRealmListReceived.RemoveAll(this);

	// Realm select result: navigate to lobby level
	Net->OnRealmSelectResult.AddRaw(this, &SAuthMasterWidget::HandleRealmSelectResult);

	// Realm list received: show RealmSelectWidget
	Net->OnRealmListReceived.AddRaw(this, &SAuthMasterWidget::HandleRealmListReceived);

	// Request realm list
	Net->RequestRealmList();
}

void SAuthMasterWidget::HandleGatewayLoginResult(const int32 Result)
{
	if (LoginPanel.IsValid())
	{
		LoginPanel->HandleGatewayLoginResult(Result);
	}
}

void SAuthMasterWidget::HandleHttpLoginError(const int32 StatusCode, const FString& Message)
{
	if (LoginPanel.IsValid())
	{
		LoginPanel->HandleHttpLoginError(StatusCode, Message);
	}
}

void SAuthMasterWidget::HandleEmailVerificationRequired(const FString& Message, const FString& Email)
{
	if (LoginPanel.IsValid())
	{
		LoginPanel->HandleEmailVerificationRequired(Message, Email);
	}
}

void SAuthMasterWidget::HandleRealmListReceived(const TArray<FRealmServerInfo>& RealmList)
{
	// Hide auth UI, show realm select
	SetVisibility(EVisibility::Collapsed);

	TSharedRef<SRealmSelectWidget> SelectWidget = SNew(SRealmSelectWidget).RealmList(RealmList);
	RealmSelectWidgetRef = SelectWidget;

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(SelectWidget);
	}
}

void SAuthMasterWidget::HandleRealmSelectResult(const int32 Result)
{
	if (Result == 0)
	{
		if (RealmSelectWidgetRef.IsValid() && GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(RealmSelectWidgetRef.ToSharedRef());
			RealmSelectWidgetRef.Reset();
		}

		if (GEngine && GEngine->GameViewport)
		{
			if (const UWorld* World = GEngine->GameViewport->GetWorld())
			{
				if (UGameInstance* GI = World->GetGameInstance())
				{
					UGameplayStatics::OpenLevel(GI, FName(TEXT("/Game/Levels/L_Lobby")), true, TEXT("?Game=/Script/DH1_Client.LobbyGameMode"));
				}
			}
		}
	}
}

UClientNetSubsystem* SAuthMasterWidget::GetNetSubsystem() const
{
	if (GEngine == nullptr || GEngine->GameViewport == nullptr)
	{
		return nullptr;
	}

	const UWorld* World = GEngine->GameViewport->GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (GI == nullptr)
	{
		return nullptr;
	}

	return GI->GetSubsystem<UClientNetSubsystem>();
}
