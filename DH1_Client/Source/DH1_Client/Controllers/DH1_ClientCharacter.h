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

	UClientNetSubsystem* GetNetSubsystem() const;

	// Enhanced Input (런타임 생성)
	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ClickMoveAction;

	// 마우스 클릭 이동
	FVector ClickMoveTarget = FVector::ZeroVector;
	bool bIsClickMoving = false;

	// 키보드 이동
	FVector CurrentMoveDirection = FVector::ZeroVector;
	bool bIsKeyMoving = false;

	// 패킷 쓰로틀
	float MovePacketTimer = 0.0f;
	static constexpr float MovePacketInterval = 0.05f;

	// 스폰 위치 폴링 타이머
	FTimerHandle SpawnPositionTimerHandle;
};
