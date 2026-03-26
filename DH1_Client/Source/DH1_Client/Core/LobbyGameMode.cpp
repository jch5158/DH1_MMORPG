#include "LobbyGameMode.h"
#include "Controllers/DH1_ClientCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"

ALobbyGameMode::ALobbyGameMode()
{
	DefaultPawnClass = ADH1_ClientCharacter::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
}

void ALobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	SpawnFloor();

	// PlayerStart가 없으면 생성
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

	if (PlayerStarts.Num() == 0)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = TEXT("AutoPlayerStart");
		GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0.0f, 0.0f, 100.0f), FRotator::ZeroRotator, SpawnParams);
	}
}

void ALobbyGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (NewPlayer)
	{
		// UI Only에서 Game+UI로 전환
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		NewPlayer->SetInputMode(InputMode);
		NewPlayer->bShowMouseCursor = true;
	}

	UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode - HandleStartingNewPlayer, Pawn: %s"),
		NewPlayer->GetPawn() ? *NewPlayer->GetPawn()->GetName() : TEXT("None"));
}

void ALobbyGameMode::SpawnFloor()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 기본 바닥 메쉬 (엔진 기본 Plane)
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh == nullptr)
	{
		return;
	}

	// 바닥 액터 생성 (5000x5000 크기)
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = TEXT("AutoFloor");

	AStaticMeshActor* FloorActor = World->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParams);

	if (FloorActor != nullptr)
	{
		UStaticMeshComponent* MeshComp = FloorActor->GetStaticMeshComponent();
		if (MeshComp != nullptr)
		{
			MeshComp->SetStaticMesh(PlaneMesh);
			MeshComp->SetWorldScale3D(FVector(500.0f, 500.0f, 1.0f)); // 50000x50000 유닛
			MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
		}
	}
}
