#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/IHttpRequest.h"
#include "SignUpWidget.generated.h"

class UTextBlock;
class UButton;
class UEditableTextBox;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGoToLoginDelegate, const FString&, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGoToEmailVerificationDelegate, const FString&, const FString&);

UCLASS()
class DH1_CLIENT_API USignUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	FOnGoToLoginDelegate OnGoToLogin;
	FOnGoToEmailVerificationDelegate OnGoToEmailVerification;

	void ResetSignUpWidget() const;
	void SetStatusTextMessage(const FString& StatusText) const;

protected:
	virtual void NativeConstruct() override;

	// 에러 메시지 출력용 텍스트 블록 (UMG에서 추가해야 함)
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusTextMessage;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* EmailInput;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* PasswordInput;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* PasswordConfirmInput;

	UPROPERTY(meta = (BindWidget))
	UButton* SignUpButton;

	UPROPERTY(meta = (BindWidget))
	UButton* BackLoginButton;

	UFUNCTION()
	void OnSignUpButtonClicked();

	UFUNCTION()
	void OnBackLoginButtonClicked();
	
	void OnSignUpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
