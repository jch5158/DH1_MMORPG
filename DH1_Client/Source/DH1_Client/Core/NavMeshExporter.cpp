// Copyright DH1 MMORPG Project. All Rights Reserved.

#include "NavMeshExporter.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "Detour/DetourNavMesh.h"

namespace
{
	static constexpr int32 NAVMESH_SET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';
	static constexpr int32 NAVMESH_SET_VERSION = 1;

	struct NavMeshSetHeader
	{
		int32 Magic;
		int32 Version;
		int32 NumTiles;
		dtNavMeshParams Params;
	};

	struct NavMeshTileHeader
	{
		dtTileRef TileRef;
		int32 DataSize;
	};
}

bool UNavMeshExporter::ExportNavMeshToFile(const UObject* WorldContextObject, const FString& OutputPath)
{
	if (WorldContextObject == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NavMeshExporter - WorldContextObject is null"));
		return false;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NavMeshExporter - World is null"));
		return false;
	}

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (NavSys == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NavMeshExporter - NavigationSystem not found"));
		return false;
	}

	const ARecastNavMesh* RecastNavMesh = nullptr;
	for (const ANavigationData* NavData : NavSys->NavDataSet)
	{
		RecastNavMesh = Cast<ARecastNavMesh>(NavData);
		if (RecastNavMesh != nullptr)
		{
			break;
		}
	}

	if (RecastNavMesh == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NavMeshExporter - RecastNavMesh not found. Add a NavMeshBoundsVolume to the level."));
		return false;
	}

	const dtNavMesh* DetourNavMesh = RecastNavMesh->GetRecastMesh();
	if (DetourNavMesh == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NavMeshExporter - Internal dtNavMesh is null. Build NavMesh first."));
		return false;
	}

	// 출력 디렉토리 생성
	const FString Directory = FPaths::GetPath(OutputPath);
	if (!FPaths::DirectoryExists(Directory))
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
	}

	FILE* fp = nullptr;
	if (fopen_s(&fp, TCHAR_TO_UTF8(*OutputPath), "wb") != 0 || fp == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NavMeshExporter - Failed to create file: %s"), *OutputPath);
		return false;
	}

	// 타일 수 카운트
	const dtNavMeshParams* NavParams = DetourNavMesh->getParams();
	int32 NumTiles = 0;
	for (int32 i = 0; i < DetourNavMesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* Tile = DetourNavMesh->getTile(i);
		if (Tile == nullptr || Tile->header == nullptr || Tile->dataSize == 0)
		{
			continue;
		}
		++NumTiles;
	}

	// 헤더 작성
	NavMeshSetHeader Header;
	Header.Magic = NAVMESH_SET_MAGIC;
	Header.Version = NAVMESH_SET_VERSION;
	Header.NumTiles = NumTiles;
	FMemory::Memcpy(&Header.Params, NavParams, sizeof(dtNavMeshParams));
	fwrite(&Header, sizeof(NavMeshSetHeader), 1, fp);

	// 타일 데이터 작성
	for (int32 i = 0; i < DetourNavMesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* Tile = DetourNavMesh->getTile(i);
		if (Tile == nullptr || Tile->header == nullptr || Tile->dataSize == 0)
		{
			continue;
		}

		NavMeshTileHeader TileHeader;
		TileHeader.TileRef = DetourNavMesh->getTileRef(Tile);
		TileHeader.DataSize = Tile->dataSize;

		fwrite(&TileHeader, sizeof(NavMeshTileHeader), 1, fp);
		fwrite(Tile->data, Tile->dataSize, 1, fp);
	}

	fclose(fp);

	UE_LOG(LogTemp, Warning, TEXT("NavMeshExporter - Exported %d tiles to: %s"), NumTiles, *OutputPath);
	return true;
}
