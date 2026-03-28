// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DH1_ClientCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UClientNetSubsystem;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;

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


public:

	/** Constructor */
	ADH1_ClientCharacter();

	/** Initialization */
	virtual void BeginPlay() override;

	/** Input Setup */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Update */
	virtual void Tick(float DeltaSeconds) override;

	/** Returns the camera component **/
	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }

	/** Returns the Camera Boom component **/
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

private:

	void CreateInputAssets();
	void OnMoveInput(const FInputActionValue& Value);
	void OnMoveInputCompleted();
	void OnClickMove();
	void OnRightMousePressed(const FInputActionValue& Value);
	void OnRightMouseReleased(const FInputActionValue& Value);

	UClientNetSubsystem* GetNetSubsystem() const;

	// Enhanced Input (런타임 생성)
	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ClickMoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> RightMouseAction;

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
