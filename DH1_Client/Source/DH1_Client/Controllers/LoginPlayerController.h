#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LoginPlayerController.generated.h"

class UUserWidget;
class SAuthMasterWidget;

UCLASS()
class DH1_CLIENT_API ALoginPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALoginPlayerController();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleChangePasswordClicked();

	UFUNCTION()
	void HandleCreateAccountClicked();

	UFUNCTION()
	void HandleBackToLoginClicked();

	UFUNCTION()
	void HandleOpenEmailVerificationClicked();

	UFUNCTION()
	void HandleResetSubmitClicked();

	UFUNCTION()
	void HandleSignupSubmitClicked();

	UFUNCTION()
	void HandleVerifyEmailClicked();

private:
	bool ExecuteFirstAvailableAuthFunction(const TArray<FName>& CandidateFunctions) const;

	UPROPERTY()
	TObjectPtr<UUserWidget> AuthWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> BackgroundWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> AuthWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> BackgroundWidgetClass;

	TSharedPtr<SAuthMasterWidget> SlateAuthWidget;
};
