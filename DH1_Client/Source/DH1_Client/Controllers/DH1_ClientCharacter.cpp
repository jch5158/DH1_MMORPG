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
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "UI/UMG/CharacterOverheadWidget.h"
#include "UI/UMG/ChatPanelWidget.h"
#include "UI/AuthWidgetStyle.h"
#include "UI/SCharacterCreatePanel.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterMove, Log, All);

ADH1_ClientCharacter::ADH1_ClientCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = false;
	GetCharacterMovement()->bSnapToPlaneAtStart = false;
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.3f;

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

	// 쿼터뷰 카메라 붐
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 1800.f;
	CameraBoom->SetRelativeRotation(FRotator(-45.f, 45.f, 0.f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f;

	// 쿼터뷰 카메라
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidgetComponent->SetupAttachment(GetCapsuleComponent());
	OverheadWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 92.f));
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	OverheadWidgetComponent->SetDrawAtDesiredSize(true);
	OverheadWidgetComponent->SetPivot(FVector2D(0.5f, 1.f));
	OverheadWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverheadWidgetComponent->SetVisibility(false);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ADH1_ClientCharacter::CreateInputAssets()
{
	ClickMoveAction = NewObject<UInputAction>(this, TEXT("IA_ClickMove"));
	ClickMoveAction->ValueType = EInputActionValueType::Boolean;

	ZoomAction = NewObject<UInputAction>(this, TEXT("IA_Zoom"));
	ZoomAction->ValueType = EInputActionValueType::Axis1D;

	JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"));
	JumpAction->ValueType = EInputActionValueType::Boolean;

	ChatFocusAction = NewObject<UInputAction>(this, TEXT("IA_ChatFocus"));
	ChatFocusAction->ValueType = EInputActionValueType::Boolean;

	ChatChannelCycleAction = NewObject<UInputAction>(this, TEXT("IA_ChatChannelCycle"));
	ChatChannelCycleAction->ValueType = EInputActionValueType::Boolean;

	EscapeAction = NewObject<UInputAction>(this, TEXT("IA_Escape"));
	EscapeAction->ValueType = EInputActionValueType::Boolean;

	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));
	DefaultMappingContext->MapKey(ClickMoveAction, EKeys::RightMouseButton);
	DefaultMappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);
	DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	DefaultMappingContext->MapKey(ChatFocusAction, EKeys::Enter);
	DefaultMappingContext->MapKey(ChatChannelCycleAction, EKeys::Tab);
	DefaultMappingContext->MapKey(EscapeAction, EKeys::Escape);
}

void ADH1_ClientCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter::BeginPlay - Location: %s, Controller: %s"),
		*GetActorLocation().ToString(), Controller ? *Controller->GetName() : TEXT("None"));

	CreateInputAssets();
	SetupCharacterOverhead();

	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - MappingContext added"));
		}

		// 마우스 커서 표시 + GameAndUI 입력 모드 (클릭 이동 + 카메라 회전 동시 지원)
		APlayerController* MutablePC = const_cast<APlayerController*>(PC);
		MutablePC->bShowMouseCursor = true;
		MutablePC->DefaultMouseCursor = EMouseCursor::Default;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		MutablePC->SetInputMode(InputMode);

		// 서버 위치 수신 전까지 캐릭터 숨기고 낙하 방지
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		GetCharacterMovement()->GravityScale = 0.0f;
		GetCharacterMovement()->StopMovementImmediately();

		// delegate 바인딩
		if (UClientNetSubsystem* NetSub = GetNetSubsystem())
		{
			NetSub->OnSpawnPositionReceived.AddUObject(this, &ADH1_ClientCharacter::OnSpawnPositionReceived);
			NetSub->OnMovePathReceived.AddUObject(this, &ADH1_ClientCharacter::OnMovePathReceived);
			NetSub->OnPositionCorrection.AddUObject(this, &ADH1_ClientCharacter::OnPositionCorrected);
			NetSub->OnCharacterCreateRequired.AddUObject(this, &ADH1_ClientCharacter::OnCharacterCreateRequired);
			NetSub->OnCharacterCreateResult.AddUObject(this, &ADH1_ClientCharacter::OnCharacterCreateResult);

			if (NetSub->IsCharacterCreatePending())
			{
				UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter::BeginPlay - CharacterCreatePending detected, showing create panel"));
				ShowCharacterCreatePanel();
			}
			else
			{
				NetSub->RequestSpawnPosition();
			}
		}
	}
	else
	{
		UE_LOG(LogCharacterMove, Error, TEXT("DH1_ClientCharacter::BeginPlay - No PlayerController!"));
	}
}

void ADH1_ClientCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (ClickMoveAction == nullptr)
	{
		CreateInputAssets();
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ClickMoveAction)
		{
			EnhancedInput->BindAction(ClickMoveAction, ETriggerEvent::Started, this, &ADH1_ClientCharacter::OnClickMove);
		}

		if (ZoomAction)
		{
			EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ADH1_ClientCharacter::OnZoom);
		}

		if (JumpAction)
		{
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ADH1_ClientCharacter::OnJumpStarted);
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADH1_ClientCharacter::OnJumpStopped);
		}

		if (ChatFocusAction)
		{
			EnhancedInput->BindAction(ChatFocusAction, ETriggerEvent::Started, this, &ADH1_ClientCharacter::OnChatFocusPressed);
		}

		if (ChatChannelCycleAction)
		{
			EnhancedInput->BindAction(ChatChannelCycleAction, ETriggerEvent::Started, this, &ADH1_ClientCharacter::OnChatChannelCycle);
		}

		if (EscapeAction)
		{
			EnhancedInput->BindAction(EscapeAction, ETriggerEvent::Started, this, &ADH1_ClientCharacter::OnEscapePressed);
		}
	}
}

void ADH1_ClientCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 위치 보정 보간
	if (bIsCorrecting)
	{
		CorrectionAlpha += DeltaSeconds * 5.0f; // ~200ms
		const FVector CurrentPos = GetActorLocation();
		const FVector NewPos = FMath::Lerp(CurrentPos, CorrectionTarget, FMath::Clamp(CorrectionAlpha, 0.0f, 1.0f));
		SetActorLocation(NewPos);

		if (CorrectionAlpha >= 1.0f)
		{
			bIsCorrecting = false;
		}
		return;
	}

	// 서버 경로 이동 (클릭 이동의 서버 응답)
	if (bIsServerPathMoving)
	{
		FollowServerPath(DeltaSeconds);
		return;
	}

	// 마우스 클릭 이동 처리
	if (bIsClickMoving)
	{
		const FVector CurrentLocation = GetActorLocation();
		const float DistanceToTarget = FVector::Dist2D(CurrentLocation, ClickMoveTarget);

		if (DistanceToTarget < 50.0f)
		{
			bIsClickMoving = false;
			CurrentMoveDirection = FVector::ZeroVector;
		}
		else
		{
			CurrentMoveDirection = (ClickMoveTarget - CurrentLocation).GetSafeNormal2D();
		}
	}

	// 쿼터뷰 줌 부드러운 보간
	if (CameraBoom)
	{
		const float Current = CameraBoom->TargetArmLength;
		if (!FMath::IsNearlyEqual(Current, TargetArmLength, 1.f))
		{
			CameraBoom->TargetArmLength = FMath::FInterpTo(Current, TargetArmLength, DeltaSeconds, ZoomInterpSpeed);
		}
	}

	// 로컬 이동 (즉시 반영)
	if (!CurrentMoveDirection.IsNearlyZero())
	{
		AddMovementInput(CurrentMoveDirection, 1.0f);
	}
}

void ADH1_ClientCharacter::OnZoom(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	TargetArmLength = FMath::Clamp(TargetArmLength - Axis * ZoomStep, MinArmLength, MaxArmLength);
}

void ADH1_ClientCharacter::OnJumpStarted()
{
	Jump();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UClientNetSubsystem* Net = GI->GetSubsystem<UClientNetSubsystem>())
		{
			Net->SendJumpNotify();
		}
	}
}

void ADH1_ClientCharacter::OnJumpStopped()
{
	StopJumping();
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
		bIsKeyMoving = false;

		// 서버에 현재 위치 + 목적지 전송 (서버가 경로 계산)
		if (UClientNetSubsystem* NetSub = GetNetSubsystem())
		{
			NetSub->SendMoveToPosition(GetActorLocation(), ClickMoveTarget);
		}

		// 서버 응답 전까지 목적지 방향으로 로컬 이동 시작 (예측)
		bIsClickMoving = true;
		CurrentMoveDirection = (ClickMoveTarget - GetActorLocation()).GetSafeNormal2D();

		UE_LOG(LogCharacterMove, Log, TEXT("ClickMove - Target: (%.1f, %.1f, %.1f)"),
			ClickMoveTarget.X, ClickMoveTarget.Y, ClickMoveTarget.Z);
	}
}

void ADH1_ClientCharacter::OnSpawnPositionReceived(const FVector& Position, const float Yaw)
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC != nullptr && !PC->IsLocalPlayerController())
	{
		return;
	}

	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		FString SheetName;
		int32 SheetLevel = 1;
		float SheetCur = 100.f;
		float SheetMax = 100.f;
		if (NetSub->ConsumePendingSpawnCharacterSheet(SheetName, SheetLevel, SheetCur, SheetMax))
		{
			SetOverheadDisplayData(SheetName, SheetLevel, SheetCur, SheetMax);
		}
	}

	SetActorLocation(Position);
	SetActorRotation(FRotator(0.0f, Yaw, 0.0f));

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	GetCharacterMovement()->GravityScale = 1.0f;

	if (OverheadWidgetComponent)
	{
		OverheadWidgetComponent->SetVisibility(true);
	}
	PushOverheadToWidget();

	CreateAndRegisterChatWidget();

	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		NetSub->OnSpawnPositionReceived.RemoveAll(this);
	}

	UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - Spawn position applied: %s, yaw: %.1f"),
		*Position.ToString(), Yaw);
}

void ADH1_ClientCharacter::OnMovePathReceived(const uint32 SeqId, const TArray<FVector>& Waypoints, const float MoveSpeed)
{
	if (Waypoints.Num() == 0)
	{
		bIsClickMoving = false;
		bIsServerPathMoving = false;
		CurrentMoveDirection = FVector::ZeroVector;
		UE_LOG(LogCharacterMove, Warning, TEXT("OnMovePathReceived - Empty path (no valid route), stopping. seq: %u"), SeqId);
		return;
	}

	// 클릭 이동 로컬 예측 중단 (유효한 서버 경로로 전환)
	bIsClickMoving = false;
	CurrentMoveDirection = FVector::ZeroVector;

	ServerPath = Waypoints;
	CurrentPathIndex = 0;
	bIsServerPathMoving = true;

	if (Waypoints.Num() > 0)
	{
		ClickMoveTarget = Waypoints.Last();
	}

	// 서버 이동 속도 적용 (0이면 기본값 유지)
	if (MoveSpeed > 0.0f)
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}

	UE_LOG(LogCharacterMove, Log, TEXT("OnMovePathReceived - seq: %u, waypoints: %d, speed: %.1f"),
		SeqId, Waypoints.Num(), MoveSpeed);
}

void ADH1_ClientCharacter::OnPositionCorrected(const FVector& CorrectedPosition)
{
	CorrectionTarget = CorrectedPosition;
	bIsCorrecting = true;
	CorrectionAlpha = 0.0f;

	// WASD 이동 중단
	CurrentMoveDirection = FVector::ZeroVector;
	bIsKeyMoving = false;

	UE_LOG(LogCharacterMove, Warning, TEXT("OnPositionCorrected - target: %s"), *CorrectedPosition.ToString());
}

void ADH1_ClientCharacter::FollowServerPath(const float DeltaSeconds)
{
	if (CurrentPathIndex < 0 || CurrentPathIndex >= ServerPath.Num())
	{
		bIsServerPathMoving = false;
		CurrentMoveDirection = FVector::ZeroVector;
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector Target = ServerPath[CurrentPathIndex];
	const float Distance = FVector::Dist2D(CurrentLocation, Target);

	if (Distance < 50.0f)
	{
		++CurrentPathIndex;
		if (CurrentPathIndex >= ServerPath.Num())
		{
			// 서버 경로 완료 — 목적지 미도달 시 로컬 예측으로 마무리
			bIsServerPathMoving = false;
			CurrentMoveDirection = FVector::ZeroVector;

			const float RemainDist = FVector::Dist2D(GetActorLocation(), ClickMoveTarget);
			if (RemainDist > 50.0f)
			{
				bIsClickMoving = true;
			}
			return;
		}
	}

	// 현재 웨이포인트를 향해 이동
	const FVector NextTarget = ServerPath[CurrentPathIndex];
	CurrentMoveDirection = (NextTarget - CurrentLocation).GetSafeNormal2D();
	AddMovementInput(CurrentMoveDirection, 1.0f);
}

void ADH1_ClientCharacter::OnChatFocusPressed()
{
	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		NetSub->FocusChatInput();
	}
}

void ADH1_ClientCharacter::OnChatChannelCycle()
{
	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		NetSub->CycleChatChannel();
	}
}

void ADH1_ClientCharacter::OnEscapePressed()
{
	if (bEscapeMenuVisible)
	{
		HideEscapeMenu();
	}
	else
	{
		ShowEscapeMenu();
	}
}

void ADH1_ClientCharacter::ShowEscapeMenu()
{
	if (bEscapeMenuVisible) return;
	if (GEngine == nullptr || GEngine->GameViewport == nullptr) return;

	bEscapeMenuVisible = true;

	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 20);
	const FSlateFontInfo ButtonFont = FCoreStyle::GetDefaultFontStyle("Regular", 16);

	TSharedRef<SWidget> ConfirmBtn = SNew(SBox)
		.WidthOverride(200.f)
		.HeightOverride(48.f)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked_Lambda([this]() -> FReply
			{
				ConfirmReturnToLogin();
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("DH1", "EscMenuConfirm", "확인"))
				.Font(ButtonFont)
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
				.Justification(ETextJustify::Center)
			]
		];

	TSharedRef<SWidget> CancelBtn = SNew(SBox)
		.WidthOverride(200.f)
		.HeightOverride(48.f)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked_Lambda([this]() -> FReply
			{
				HideEscapeMenu();
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("DH1", "EscMenuCancel", "취소"))
				.Font(ButtonFont)
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
				.Justification(ETextJustify::Center)
			]
		];

	TSharedRef<SWidget> ButtonRow = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 12.f, 0.f)
		[
			ConfirmBtn
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(12.f, 0.f, 0.f, 0.f)
		[
			CancelBtn
		];

	TSharedRef<SWidget> DialogBox = SNew(SBorder)
		.Padding(FMargin(40.f, 30.f))
		.BorderImage(AuthStyle::FlatBrush())
		.BorderBackgroundColor(FLinearColor(0.05f, 0.06f, 0.08f, 0.92f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, 24.f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("DH1", "EscMenuTitle", "로그인 화면으로 돌아가시겠습니까?"))
				.Font(TitleFont)
				.ColorAndOpacity(FLinearColor(0.95f, 0.95f, 0.97f, 1.f))
				.Justification(ETextJustify::Center)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				ButtonRow
			]
		];

	TSharedRef<SWidget> Overlay = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Padding(FMargin(0))
			.BorderImage(AuthStyle::FlatBrush())
			.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.04f, 0.75f))
			[
				SNew(SBox)
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			DialogBox
		];

	EscapeMenuOverlayRef = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(Overlay, 100);
}

void ADH1_ClientCharacter::HideEscapeMenu()
{
	if (!bEscapeMenuVisible) return;
	bEscapeMenuVisible = false;

	if (EscapeMenuOverlayRef.IsValid() && GEngine != nullptr && GEngine->GameViewport != nullptr)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(EscapeMenuOverlayRef.ToSharedRef());
	}
	EscapeMenuOverlayRef.Reset();
}

void ADH1_ClientCharacter::ConfirmReturnToLogin()
{
	HideEscapeMenu();

	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		NetSub->Disconnect();
	}

	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Levels/L_Login")));
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

void ADH1_ClientCharacter::SetupCharacterOverhead()
{
	if (OverheadWidgetComponent == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	const TSubclassOf<UCharacterOverheadWidget> ClassToUse =
		OverheadWidgetClass ? OverheadWidgetClass.Get() : UCharacterOverheadWidget::StaticClass();

	OverheadWidgetInstance = CreateWidget<UCharacterOverheadWidget>(GetWorld(), ClassToUse);
	if (OverheadWidgetInstance == nullptr)
	{
		return;
	}

	OverheadWidgetComponent->SetWidget(OverheadWidgetInstance);
	OverheadWidgetInstance->SetIsLocalPlayer(true);
	PushOverheadToWidget();

	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		NetSub->OnCharacterOverheadData.AddUObject(this, &ADH1_ClientCharacter::OnCharacterOverheadDataFromNet);
	}
}

void ADH1_ClientCharacter::PushOverheadToWidget()
{
	if (OverheadWidgetInstance == nullptr)
	{
		return;
	}

	OverheadWidgetInstance->SetOverheadData(OverheadDisplayName, OverheadLevel, OverheadCurrentHP, OverheadMaxHP);
}

void ADH1_ClientCharacter::SetOverheadDisplayData(const FString& DisplayName, const int32 Level, const float CurrentHP, const float MaxHP)
{
	OverheadDisplayName = DisplayName.IsEmpty() ? TEXT("Adventurer") : DisplayName;
	OverheadLevel = FMath::Clamp(FMath::Max(1, Level), 1, 9999);
	OverheadMaxHP = FMath::Max(1.f, MaxHP);
	OverheadCurrentHP = FMath::Clamp(CurrentHP, 0.f, OverheadMaxHP);
	PushOverheadToWidget();
}

void ADH1_ClientCharacter::OnCharacterOverheadDataFromNet(const FString& Name, const int32 Level, const float CurrentHP, const float MaxHP)
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr || !PC->IsLocalPlayerController())
	{
		return;
	}

	SetOverheadDisplayData(Name, Level, CurrentHP, MaxHP);
}

void ADH1_ClientCharacter::CreateAndRegisterChatWidget()
{
	if (ChatWidgetInstance != nullptr)
	{
		UE_LOG(LogCharacterMove, Warning, TEXT("CreateChat: already exists"));
		return;
	}

	UWorld* W = GetWorld();
	if (W == nullptr)
	{
		UE_LOG(LogCharacterMove, Error, TEXT("CreateChat: World is null"));
		return;
	}

	UClientNetSubsystem* NetSub = GetNetSubsystem();
	if (NetSub == nullptr)
	{
		UE_LOG(LogCharacterMove, Error, TEXT("CreateChat: NetSub is null"));
		return;
	}

	ChatWidgetInstance = CreateWidget<UChatPanelWidget>(W);
	if (ChatWidgetInstance == nullptr)
	{
		UE_LOG(LogCharacterMove, Error, TEXT("CreateChat: CreateWidget<UChatPanelWidget> returned null"));
		return;
	}
	if (ChatWidgetInstance->WidgetTree == nullptr)
	{
		UE_LOG(LogCharacterMove, Error, TEXT("CreateChat: WidgetTree is null"));
		return;
	}
	UE_LOG(LogCharacterMove, Warning, TEXT("CreateChat: widget created, building tree..."));

	UWidgetTree* Tree = ChatWidgetInstance->WidgetTree;

	UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ChatRoot"));
	Tree->RootWidget = Root;

	UBorder* BgBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChatBg"));
	Root->AddChildToCanvas(BgBorder);
	BgBorder->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.06f, 0.72f));
	BgBorder->SetPadding(FMargin(6.f));
	if (UCanvasPanelSlot* BgSlot = Cast<UCanvasPanelSlot>(BgBorder->Slot))
	{
		BgSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		BgSlot->SetAlignment(FVector2D(0.f, 1.f));
		BgSlot->SetPosition(FVector2D(12.f, -12.f));
		BgSlot->SetSize(FVector2D(540.f, 260.f));
		BgSlot->SetAutoSize(false);
	}

	UVerticalBox* MainVBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChatMainVBox"));
	BgBorder->SetContent(MainVBox);

	UScrollBox* ScrollLog = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Scroll_Log"));
	MainVBox->AddChildToVerticalBox(ScrollLog);
	if (UVerticalBoxSlot* ScrollSlot = Cast<UVerticalBoxSlot>(ScrollLog->Slot))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UVerticalBox* LogLines = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox_LogLines"));
	ScrollLog->AddChild(LogLines);

	UHorizontalBox* InputRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InputRow"));
	MainVBox->AddChildToVerticalBox(InputRow);
	if (UVerticalBoxSlot* InputSlot = Cast<UVerticalBoxSlot>(InputRow->Slot))
	{
		InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		InputSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}

	USizeBox* ComboSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ComboSizeBox"));
	ComboSizeBox->SetWidthOverride(82.f);
	InputRow->AddChildToHorizontalBox(ComboSizeBox);
	if (UHorizontalBoxSlot* ComboSlot = Cast<UHorizontalBoxSlot>(ComboSizeBox->Slot))
	{
		ComboSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		ComboSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
	}

	UComboBoxString* Combo = Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("Combo_Channel"));
	Combo->Font = FCoreStyle::GetDefaultFontStyle("Regular", 12);
	Combo->ForegroundColor = FSlateColor(FLinearColor::White);
	ComboSizeBox->AddChild(Combo);

	UEditableTextBox* Edit = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("EditableText_Message"));
	{
		FEditableTextBoxStyle EditStyle = Edit->GetWidgetStyle();
		const FSlateColor InputBg(FLinearColor(0.06f, 0.07f, 0.12f, 0.9f));
		const FSlateColor InputBgFocus(FLinearColor(0.08f, 0.10f, 0.18f, 0.95f));
		EditStyle.BackgroundImageNormal.TintColor = InputBg;
		EditStyle.BackgroundImageHovered.TintColor = InputBg;
		EditStyle.BackgroundImageFocused.TintColor = InputBgFocus;
		EditStyle.BackgroundImageReadOnly.TintColor = InputBg;
		EditStyle.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
		EditStyle.ForegroundColor = FSlateColor(FLinearColor(0.92f, 0.94f, 1.f));
		EditStyle.TextStyle.ColorAndOpacity = FSlateColor(FLinearColor(0.92f, 0.94f, 1.f));
		EditStyle.Padding = FMargin(6.f, 4.f);
		Edit->SetWidgetStyle(EditStyle);
		Edit->SetHintText(FText::FromString(TEXT("메시지를 입력하세요...")));
	}
	InputRow->AddChildToHorizontalBox(Edit);
	if (UHorizontalBoxSlot* EditSlot = Cast<UHorizontalBoxSlot>(Edit->Slot))
	{
		EditSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UButton* SendBtn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_Send"));
	InputRow->AddChildToHorizontalBox(SendBtn);
	{
		FButtonStyle BtnStyle = SendBtn->GetStyle();
		BtnStyle.Normal.TintColor  = FSlateColor(FLinearColor(0.12f, 0.20f, 0.38f, 1.f));
		BtnStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.18f, 0.28f, 0.50f, 1.f));
		BtnStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.10f, 0.16f, 0.32f, 1.f));
		BtnStyle.NormalForeground  = FSlateColor(FLinearColor::White);
		BtnStyle.HoveredForeground = FSlateColor(FLinearColor::White);
		BtnStyle.PressedForeground = FSlateColor(FLinearColor::White);
		BtnStyle.NormalPadding  = FMargin(10.f, 4.f);
		BtnStyle.PressedPadding = FMargin(10.f, 5.f);
		SendBtn->SetStyle(BtnStyle);
	}
	UTextBlock* SendLabel = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SendLabel"));
	SendBtn->SetContent(SendLabel);
	SendLabel->SetText(FText::FromString(TEXT("전송")));
	SendLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
	SendLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UHorizontalBoxSlot* BtnSlot = Cast<UHorizontalBoxSlot>(SendBtn->Slot))
	{
		BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		BtnSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));
	}

	ChatWidgetInstance->AddToViewport(50);
	NetSub->RegisterChatUi(ChatWidgetInstance);

	UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - Chat widget created and registered"));
}

void ADH1_ClientCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideEscapeMenu();
	HideCharacterCreatePanel();

	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		NetSub->UnregisterChatUi();
		NetSub->OnCharacterOverheadData.RemoveAll(this);
		NetSub->OnSpawnPositionReceived.RemoveAll(this);
		NetSub->OnCharacterCreateRequired.RemoveAll(this);
		NetSub->OnCharacterCreateResult.RemoveAll(this);
	}

	if (ChatWidgetInstance != nullptr)
	{
		ChatWidgetInstance->RemoveFromParent();
		ChatWidgetInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ADH1_ClientCharacter::OnCharacterCreateRequired()
{
	UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - Character creation required"));
	ShowCharacterCreatePanel();
}

void ADH1_ClientCharacter::OnCharacterCreateResult(const int32 Result, const FString& Message)
{
	UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - Character create result: %d, msg: %s"), Result, *Message);

	if (Result == 0)
	{
		if (UClientNetSubsystem* NetSub = GetNetSubsystem())
		{
			NetSub->ClearCharacterCreatePending();
		}

		if (CharacterCreatePanel.IsValid())
		{
			CharacterCreatePanel->SetStatusMessage(TEXT("캐릭터가 생성되었습니다!"), false);
		}

		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([this]()
		{
			HideCharacterCreatePanel();
		}), 0.8f, false);
	}
	else
	{
		if (CharacterCreatePanel.IsValid())
		{
			FString ErrorMsg = Message;
			if (ErrorMsg.IsEmpty())
			{
				switch (Result)
				{
				case 1: ErrorMsg = TEXT("닉네임이 너무 짧습니다."); break;
				case 2: ErrorMsg = TEXT("닉네임이 너무 깁니다."); break;
				case 3: ErrorMsg = TEXT("닉네임에 사용할 수 없는 문자가 포함되어 있습니다."); break;
				case 4: ErrorMsg = TEXT("이미 사용 중인 닉네임입니다."); break;
				case 5: ErrorMsg = TEXT("이미 캐릭터가 존재합니다."); break;
				default: ErrorMsg = TEXT("캐릭터 생성에 실패했습니다."); break;
				}
			}
			CharacterCreatePanel->SetStatusMessage(ErrorMsg, true);
			CharacterCreatePanel->SetCreateEnabled(true);
		}
	}
}

void ADH1_ClientCharacter::HandleCreateCharacterRequested(const FString& CharacterName)
{
	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		NetSub->SendCreateCharacterRequest(CharacterName);
	}
}

void ADH1_ClientCharacter::ShowCharacterCreatePanel()
{
	if (CharacterCreateOverlayRef.IsValid())
	{
		return;
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	static FSlateBrush OverlayBg;
	static bool bOverlayInit = false;
	if (!bOverlayInit)
	{
		bOverlayInit = true;
		OverlayBg.DrawAs = ESlateBrushDrawType::Image;
		OverlayBg.TintColor = FSlateColor(FLinearColor(0.01f, 0.01f, 0.01f, 0.88f));
	}

	CharacterCreateOverlayRef =
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(&OverlayBg)
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(CharacterCreatePanel, SCharacterCreatePanel)
			.OnCreateRequested(FOnCreateCharacterRequested::CreateUObject(this, &ADH1_ClientCharacter::HandleCreateCharacterRequested))
		];

	GEngine->GameViewport->AddViewportWidgetContent(CharacterCreateOverlayRef.ToSharedRef(), 100);
}

void ADH1_ClientCharacter::HideCharacterCreatePanel()
{
	if (CharacterCreateOverlayRef.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(CharacterCreateOverlayRef.ToSharedRef());
	}
	CharacterCreateOverlayRef.Reset();
	CharacterCreatePanel.Reset();
}
