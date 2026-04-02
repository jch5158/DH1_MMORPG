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
class SCharacterCreatePanel;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class UUserWidget;

/**
 *  Quarter-view (쿼터뷰) MMORPG character with fixed camera angle and mouse-wheel zoom.
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
	void OnClickMove();
	void OnZoom(const FInputActionValue& Value);
	void OnJumpStarted();
	void OnJumpStopped();
	void OnChatFocusPressed();
	void OnChatChannelCycle();
	void OnEscapePressed();

	void ShowEscapeMenu();
	void HideEscapeMenu();
	void ConfirmReturnToLogin();

	UClientNetSubsystem* GetNetSubsystem() const;

	void SetupCharacterOverhead();
	void PushOverheadToWidget();
	void OnCharacterOverheadDataFromNet(const FString& Name, int32 Level, float CurrentHP, float MaxHP);

	void CreateAndRegisterChatWidget();

	void OnCharacterCreateRequired();
	void OnCharacterCreateResult(int32 Result, const FString& Message);
	void HandleCreateCharacterRequested(const FString& CharacterName);
	void ShowCharacterCreatePanel();
	void HideCharacterCreatePanel();

	TSharedPtr<SCharacterCreatePanel> CharacterCreatePanel;
	TSharedPtr<SWidget> CharacterCreateOverlayRef;

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
	TObjectPtr<UInputAction> ClickMoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ChatFocusAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ChatChannelCycleAction;

	UPROPERTY()
	TObjectPtr<UInputAction> EscapeAction;

	TSharedPtr<SWidget> EscapeMenuOverlayRef;
	bool bEscapeMenuVisible = false;

	// 좌클릭 이동
	FVector ClickMoveTarget = FVector::ZeroVector;
	bool bIsClickMoving = false;

	FVector CurrentMoveDirection = FVector::ZeroVector;
	bool bIsKeyMoving = false;

	// 쿼터뷰 카메라 줌
	float TargetArmLength = 1800.f;
	static constexpr float MinArmLength = 800.f;
	static constexpr float MaxArmLength = 3000.f;
	static constexpr float ZoomStep = 150.f;
	static constexpr float ZoomInterpSpeed = 8.f;

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
