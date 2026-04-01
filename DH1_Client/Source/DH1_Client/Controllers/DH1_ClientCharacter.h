// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DH1_ClientCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UClientNetSubsystem;
class UWidgetComponent;
class UCharacterOverheadWidget;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class UUserWidget;

/**
 *  A controllable top-down perspective character
 */
UCLASS()
class ADH1_ClientCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** 머리 위 이름·레벨·체력 (로그인 HUD와 별개) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

public:

	/** Constructor */
	ADH1_ClientCharacter();

	/** Initialization */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Input Setup */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Update */
	virtual void Tick(float DeltaSeconds) override;

	/** Returns the camera component **/
	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }

	/** Returns the Camera Boom component **/
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

	/** 오버헤드 표시 데이터 (서버 연동 전 목업 / NotifyCharacterOverheadData로 갱신) */
	UFUNCTION(BlueprintCallable, Category = "Overhead")
	void SetOverheadDisplayData(const FString& DisplayName, int32 Level, float CurrentHP, float MaxHP);

private:

	void CreateInputAssets();
	void OnMoveInput(const FInputActionValue& Value);
	void OnMoveInputCompleted();
	void OnClickMove();
	void OnRightMousePressed(const FInputActionValue& Value);
	void OnRightMouseReleased(const FInputActionValue& Value);
	void OnChatFocusPressed();
	void OnChatChannelCycle();

	UClientNetSubsystem* GetNetSubsystem() const;

	void SetupCharacterOverhead();
	void PushOverheadToWidget();
	void OnCharacterOverheadDataFromNet(const FString& Name, int32 Level, float CurrentHP, float MaxHP);

	void CreateAndRegisterChatWidget();

	UPROPERTY()
	TObjectPtr<UUserWidget> ChatWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category = "Overhead")
	TSubclassOf<UCharacterOverheadWidget> OverheadWidgetClass;

	UPROPERTY()
	TObjectPtr<UCharacterOverheadWidget> OverheadWidgetInstance;

	FString OverheadDisplayName = TEXT("Adventurer");
	int32 OverheadLevel = 1;
	float OverheadCurrentHP = 100.f;
	float OverheadMaxHP = 100.f;

	// Enhanced Input (런타임 생성)
	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ClickMoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> RightMouseAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ChatFocusAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ChatChannelCycleAction;

	// 마우스 클릭 이동 (좌클릭)
	FVector ClickMoveTarget = FVector::ZeroVector;
	bool bIsClickMoving = false;

	// 키보드 이동
	FVector CurrentMoveDirection = FVector::ZeroVector;
	bool bIsKeyMoving = false;

	// 카메라 회전 (우클릭 홀드)
	bool bRightMouseHeld = false;
	float CameraYaw = 0.0f;
	float CameraPitch = -50.0f;

	// 스폰 위치 수신 콜백
	void OnSpawnPositionReceived(const FVector& Position, float Yaw);

	// 서버 경로 이동
	void OnMovePathReceived(uint32 SeqId, const TArray<FVector>& Waypoints, float MoveSpeed);
	void OnPositionCorrected(const FVector& CorrectedPosition);
	void FollowServerPath(float DeltaSeconds);

	TArray<FVector> ServerPath;
	int32 CurrentPathIndex = -1;
	bool bIsServerPathMoving = false;

	// 위치 보정 보간
	FVector CorrectionTarget = FVector::ZeroVector;
	bool bIsCorrecting = false;
	float CorrectionAlpha = 0.0f;
};
