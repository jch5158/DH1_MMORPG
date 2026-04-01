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
	// WASD/방향키 이동 (Vector2D: X=좌우, Y=전후)
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	// 마우스 왼쪽 클릭
	ClickMoveAction = NewObject<UInputAction>(this, TEXT("IA_ClickMove"));
	ClickMoveAction->ValueType = EInputActionValueType::Boolean;

	// 매핑 컨텍스트
	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));

	DefaultMappingContext->MapKey(ClickMoveAction, EKeys::RightMouseButton);
	DefaultMappingContext->MapKey(ClickMoveAction, EKeys::LeftMouseButton);

	// 카메라 회전 액션 (미사용 - 우클릭은 클릭 이동으로 예약)
	RightMouseAction = NewObject<UInputAction>(this, TEXT("IA_RightMouse"));
	RightMouseAction->ValueType = EInputActionValueType::Boolean;

	ChatFocusAction = NewObject<UInputAction>(this, TEXT("IA_ChatFocus"));
	ChatFocusAction->ValueType = EInputActionValueType::Boolean;
	DefaultMappingContext->MapKey(ChatFocusAction, EKeys::Enter);
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
			NetSub->RequestSpawnPosition();
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

	// SetupPlayerInputComponent는 BeginPlay 전에 호출되므로 여기서 Input 에셋 생성
	if (MoveAction == nullptr)
	{
		CreateInputAssets();
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ClickMoveAction != nullptr)
		{
			EnhancedInput->BindAction(ClickMoveAction, ETriggerEvent::Started, this, &ADH1_ClientCharacter::OnClickMove);
			UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - ClickMoveAction bound"));
		}

		if (RightMouseAction != nullptr)
		{
			EnhancedInput->BindAction(RightMouseAction, ETriggerEvent::Started, this, &ADH1_ClientCharacter::OnRightMousePressed);
			EnhancedInput->BindAction(RightMouseAction, ETriggerEvent::Completed, this, &ADH1_ClientCharacter::OnRightMouseReleased);
			UE_LOG(LogCharacterMove, Warning, TEXT("DH1_ClientCharacter - RightMouseAction bound"));
		}

		if (ChatFocusAction != nullptr)
		{
			EnhancedInput->BindAction(ChatFocusAction, ETriggerEvent::Started, this, &ADH1_ClientCharacter::OnChatFocusPressed);
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

	// 카메라 회전 (우클릭 홀드 + 마우스 드래그)
	if (bRightMouseHeld)
	{
		if (const APlayerController* PC = Cast<APlayerController>(Controller))
		{
			float MouseDX, MouseDY;
			PC->GetInputMouseDelta(MouseDX, MouseDY);
			CameraYaw += MouseDX * 2.0f;
			CameraPitch = FMath::Clamp(CameraPitch - MouseDY * 1.5f, -80.0f, -15.0f);
			CameraBoom->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
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
	// WASD 이동 비활성화 — 클릭이동으로 통일
}

void ADH1_ClientCharacter::OnMoveInputCompleted()
{
	// WASD 이동 비활성화 — 클릭이동으로 통일
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

	FVector Loc = Position;
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		const float HalfH = Cap->GetScaledCapsuleHalfHeight();
		if (Position.Z < HalfH + 10.f)
		{
			Loc.Z = Position.Z + HalfH;
		}
	}

	SetActorLocation(Loc);
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
		// 서버 경로 없음 — 로컬 예측 유지하여 이동 계속
		UE_LOG(LogCharacterMove, Warning, TEXT("OnMovePathReceived - Empty path, seq: %u"), SeqId);
		return;
	}

	// 클릭 이동 로컬 예측 중단 (유효한 서버 경로로 전환)
	bIsClickMoving = false;
	CurrentMoveDirection = FVector::ZeroVector;

	ServerPath = Waypoints;
	CurrentPathIndex = 0;
	bIsServerPathMoving = true;

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

void ADH1_ClientCharacter::OnRightMousePressed(const FInputActionValue& Value)
{
	bRightMouseHeld = true;

	// 우클릭 홀드 중 커서 숨김
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->bShowMouseCursor = false;
	}
}

void ADH1_ClientCharacter::OnRightMouseReleased(const FInputActionValue& Value)
{
	bRightMouseHeld = false;

	// 우클릭 해제 시 커서 복원
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->bShowMouseCursor = true;
	}
}

void ADH1_ClientCharacter::OnChatFocusPressed()
{
	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		NetSub->FocusChatInput();
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
	if (UClientNetSubsystem* NetSub = GetNetSubsystem())
	{
		NetSub->UnregisterChatUi();
		NetSub->OnCharacterOverheadData.RemoveAll(this);
		NetSub->OnSpawnPositionReceived.RemoveAll(this);
	}

	if (ChatWidgetInstance != nullptr)
	{
		ChatWidgetInstance->RemoveFromParent();
		ChatWidgetInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
