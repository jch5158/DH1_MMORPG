#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LoginPlayerController.generated.h"

UCLASS()
class DH1_CLIENT_API ALoginPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
    virtual void BeginPlay() override;

    // 블루프린트에서 방금 만든 WBP_SignUp 클래스를 할당할 수 있게 열어둠
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> AuthMasterWidgetClass;

private:
    // 화면에 띄운 위젯 인스턴스를 메모리에 유지하기 위한 포인터
    UPROPERTY()
    UUserWidget* AuthMasterWidgetInstance;
};
