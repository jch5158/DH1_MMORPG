// Copyright DH1 MMORPG Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NavMeshExporter.generated.h"

UCLASS()
class DH1_CLIENT_API UNavMeshExporter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// UE5 NavMesh를 Recast 바이너리 포맷으로 Export
	// OutputPath: 출력 파일 경로 (예: "C:/NavMesh/L_GameWorld.bin")
	UFUNCTION(BlueprintCallable, Category = "NavMesh", meta = (WorldContext = "WorldContextObject"))
	static bool ExportNavMeshToFile(const UObject* WorldContextObject, const FString& OutputPath);
};
