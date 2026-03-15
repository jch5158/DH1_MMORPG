#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/IHttpRequest.h"
#include "SignUpWidget.generated.h"

class UTextBlock;
class UButton;
class UEditableTextBox;

// 이메일 데이터를 전달하기 위한 델리게이트 선언
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSignUpSuccessDelegate, const FString& /*RegisteredEmail*/);
DECLARE_MULTICAST_DELEGATE(FOnGoToLoginDelegate); // 취소/뒤로가기 버튼용

UCLASS()
class DH1_CLIENT_API USignUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	FOnSignUpSuccessDelegate OnSignUpSuccess;
	FOnGoToLoginDelegate OnGoToLogin;

protected:

	virtual void NativeConstruct() override;

	// 블루프린트에서 할당할 WBP_Login 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> LoginWidgetClass;

	UPROPERTY(meta = (BindWidget))
	UButton* BackLoginButton;

	// 에러 메시지 출력용 텍스트 블록 (UMG에서 추가해야 함)
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ErrorTextMessage;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* EmailInput;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* PasswordInput;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* PasswordConfirmInput;

	UPROPERTY(meta = (BindWidget))
	UButton* SignUpButton;
	
	UFUNCTION()
	void OnSignUpButtonClicked();

	UFUNCTION()
	void OnBackLoginButtonClicked();
	
	void OnSignUpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
