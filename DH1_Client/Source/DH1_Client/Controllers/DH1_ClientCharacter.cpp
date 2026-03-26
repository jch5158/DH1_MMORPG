// Copyright Epic Games, Inc. All Rights Reserved.

#include "DH1_ClientCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Network/Subsystem/ClientNetSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterMove, Log, All);

ADH1_ClientCharacter::ADH1_ClientCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// 마네킹 스켈레탈 메쉬
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannequinMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannequinMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannequinMesh.Object);
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	}

	// 이동 애니메이션 블루프린트
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (AnimBP.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimBP.Class);
	}

	// 카메라 붐
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 1200.f;
	CameraBoom->SetRelativeRotation(FRotator(-50.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	// 카메라
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ADH1_ClientCharacter::CreateInputAssets()
{
	// WASD/방향키 이동 (Vector2D: X=좌우, Y=전후)
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	// 마우스 왼쪽 클릭
	ClickMoveAction = NewObject<UInputAction>(this, TEXT("IA_ClickMove"));
	ClickMoveAction->ValueType = EInputActionValueType::Boolean;

	// 매핑 컨텍스트
	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));

	// WASD 바인딩
	auto& WMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::W);
	UInputModifierSwizzleAxis* SwizzleW = NewObject<UInputModifierSwizzleAxis>(this);
	SwizzleW->Order = EInputAxisSwizzle::YXZ;
	WMapping.Modifiers.Add(SwizzleW);

	auto& SMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
	UInputModifierSwizzleAxis* SwizzleS = NewObject<UInputModifierSwizzleAxis>(this);
	SwizzleS->Order = EInputAxisSwizzle::YXZ;
	SMapping.Modifiers.Add(SwizzleS);
	UInputModifierNegate* NegateS = NewObject<UInputModifierNegate>(this);
	SMapping.Modifiers.Add(NegateS);

	DefaultMappingContext->MapKey(MoveAction, EKeys::D);

	auto& AMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
	UInputModifierNegate* NegateA = NewObject<UInputModifierNegate>(this);
	AMapping.Modifiers.Add(NegateA);

	// 방향키 바인딩
	auto& UpMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::Up);
	UInputModifierSwizzleAxis* SwizzleUp = NewObject<UInputModifierSwizzleAxis>(this);
	SwizzleUp->Order = EInputAxisSwizzle::YXZ;
	UpMapping.Modifiers.Add(SwizzleUp);

	auto& DownMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::Down);
	UInputModifierSwizzleAxis* SwizzleDown = NewObject<UInputModifierSwizzleAxis>(this);
	SwizzleDown->Order = EInputAxisSwizzle::YXZ;
	DownMapping.Modifiers.Add(SwizzleDown);
	UInputModifierNegate* NegateDown = NewObject<UInputModifierNegate>(this);
	DownMapping.Modifiers.Add(NegateDown);

	DefaultMappingContext->MapKey(MoveAction, EKeys::Right);

	auto& LeftMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::Left);
	UInputModifierNegate* NegateLeft = NewObject<UInputModifierNegate>(this);
	LeftMapping.Modifiers.Add(NegateLeft);

	// 마우스 왼쪽 클릭 바인딩
	DefaultMappingContext->MapKey(ClickMoveAction, EKeys::LeftMouseButton);
}

void ADH1_ClientCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter::BeginPlay - Location: %s, Controller: %s"),
		*GetActorLocation().ToString(), Controller ? *Controller->GetName() : TEXT("None"));

	CreateInputAssets();

	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - MappingContext added"));
		}

		// 마우스 커서 표시
		const_cast<APlayerController*>(PC)->bShowMouseCursor = true;
		const_cast<APlayerController*>(PC)->DefaultMouseCursor = EMouseCursor::Default;
	}
	else
	{
		UE_LOG(LogCharacterMove, Error, TEXT("DH1_ClientCharacter::BeginPlay - No PlayerController!"));
	}
}

void ADH1_ClientCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// SetupPlayerInputComponent는 BeginPlay 전에 호출되므로 여기서 Input 에셋 생성
	if (MoveAction == nullptr)
	{
		CreateInputAssets();
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction != nullptr)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADH1_ClientCharacter::OnMoveInput);
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ADH1_ClientCharacter::OnMoveInputCompleted);
			UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - MoveAction bound"));
		}

		if (ClickMoveAction != nullptr)
		{
			EnhancedInput->BindAction(ClickMoveAction, ETriggerEvent::Started, this, &ADH1_ClientCharacter::OnClickMove);
			UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - ClickMoveAction bound"));
		}
	}
}

void ADH1_ClientCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 마우스 클릭 이동 처리
	if (bIsClickMoving)
	{
		const FVector CurrentLocation = GetActorLocation();
		const float DistanceToTarget = FVector::Dist2D(CurrentLocation, ClickMoveTarget);

		if (DistanceToTarget < 50.0f)
		{
			bIsClickMoving = false;
			CurrentMoveDirection = FVector::ZeroVector;

			if (UClientNetSubsystem* NetSubsystem = GetNetSubsystem())
			{
				NetSubsystem->SendMoveStop();
			}
		}
		else
		{
			CurrentMoveDirection = (ClickMoveTarget - CurrentLocation).GetSafeNormal2D();
		}
	}

	// 이동 패킷 쓰로틀
	MovePacketTimer += DeltaSeconds;
	if (MovePacketTimer >= MovePacketInterval)
	{
		MovePacketTimer = 0.0f;

		if (!CurrentMoveDirection.IsNearlyZero())
		{
			UClientNetSubsystem* NetSubsystem = GetNetSubsystem();
			if (NetSubsystem != nullptr)
			{
				const float Yaw = CurrentMoveDirection.Rotation().Yaw;
				NetSubsystem->SendMoveInput(CurrentMoveDirection, Yaw);
			}
			else
			{
				UE_LOG(LogCharacterMove, Warning, TEXT("NetSubsystem is NULL - cannot send move packet"));
			}
		}
	}

	// 로컬 이동 (즉시 반영)
	if (!CurrentMoveDirection.IsNearlyZero())
	{
		AddMovementInput(CurrentMoveDirection, 1.0f);
	}
}

void ADH1_ClientCharacter::OnMoveInput(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();

	if (Input.IsNearlyZero())
	{
		return;
	}

	bIsClickMoving = false;
	bIsKeyMoving = true;

	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC == nullptr || PC->PlayerCameraManager == nullptr)
	{
		return;
	}

	const FRotator CameraRotation = PC->PlayerCameraManager->GetCameraRotation();
	const FRotator YawRotation(0.0f, CameraRotation.Yaw, 0.0f);

	const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	CurrentMoveDirection = (ForwardDir * Input.Y + RightDir * Input.X).GetSafeNormal();
}

void ADH1_ClientCharacter::OnMoveInputCompleted()
{
	if (!bIsKeyMoving)
	{
		return;
	}

	bIsKeyMoving = false;
	CurrentMoveDirection = FVector::ZeroVector;

	if (UClientNetSubsystem* NetSubsystem = GetNetSubsystem())
	{
		NetSubsystem->SendMoveStop();
	}
}

void ADH1_ClientCharacter::OnClickMove()
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC == nullptr)
	{
		return;
	}

	FHitResult HitResult;
	if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		ClickMoveTarget = HitResult.ImpactPoint;
		bIsClickMoving = true;
		bIsKeyMoving = false;

		CurrentMoveDirection = (ClickMoveTarget - GetActorLocation()).GetSafeNormal2D();

		UE_LOG(LogCharacterMove, Log, TEXT("ClickMove - Target: (%.1f, %.1f, %.1f)"),
			ClickMoveTarget.X, ClickMoveTarget.Y, ClickMoveTarget.Z);
	}
}

UClientNetSubsystem* ADH1_ClientCharacter::GetNetSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UClientNetSubsystem>();
}
