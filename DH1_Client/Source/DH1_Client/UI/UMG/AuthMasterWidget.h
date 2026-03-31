#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "AuthMasterWidget.generated.h"

UCLASS(Abstract)
class DH1_CLIENT_API UAuthMasterWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ─────────────────────────────────────────────────────────────
	// 네트워크 이벤트 → Blueprint 구현
	// ─────────────────────────────────────────────────────────────

	// 게이트웨이 로그인 결과 (0=성공)
	UFUNCTION(BlueprintImplementableEvent, Category = "Auth|Events")
	void OnGatewayLoginResult(int32 Result);

	// HTTP 로그인 오류
	UFUNCTION(BlueprintImplementableEvent, Category = "Auth|Events")
	void OnHttpLoginError(int32 StatusCode, const FString& Message);

	// 이메일 인증 필요 (bSkipAutoSendVerifyCode: 서버가 이미 인증 메일을 보낸 경우 true — UMG에서 자동 재전송 생략)
	UFUNCTION(BlueprintImplementableEvent, Category = "Auth|Events")
	void OnEmailVerificationRequired(const FString& Message, const FString& Email, bool bSkipAutoSendVerifyCode);

	// 렐름 목록 수신
	UFUNCTION(BlueprintImplementableEvent, Category = "Auth|Events")
	void OnRealmListReady(const TArray<FRealmServerInfo>& RealmList);

	// 렐름 선택 결과 (0=성공)
	UFUNCTION(BlueprintImplementableEvent, Category = "Auth|Events")
	void OnRealmSelectDone(int32 Result);

	// ─────────────────────────────────────────────────────────────
	// Blueprint에서 호출 가능한 액션
	// ─────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Auth|Actions")
	void RequestLogin(const FString& Email, const FString& Password);

	UFUNCTION(BlueprintCallable, Category = "Auth|Actions")
	void RequestRealmList();

	UFUNCTION(BlueprintCallable, Category = "Auth|Actions")
	void RequestRealmSelect(int32 RealmId);

	UFUNCTION(BlueprintCallable, Category = "Auth|Actions")
	void OpenLobbyLevel();

private:
	void HandleGatewayLoginResult(int32 Result);
	void HandleHttpLoginError(int32 StatusCode, const FString& Message);
	void HandleEmailVerificationRequired(const FString& Message, const FString& Email);
	void HandleRealmListReceived(const TArray<FRealmServerInfo>& RealmList);
	void HandleRealmSelectResult(int32 Result);

	UClientNetSubsystem* GetNetSubsystem() const;
};
