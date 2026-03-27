#include "pch.h"
#include "NavMeshManager.h"
#include <recastnavigation/DetourNavMesh.h>
#include <recastnavigation/DetourNavMeshQuery.h>

// NavMesh Export 파일 헤더 (UE5 Exporter와 동일한 포맷)
namespace
{
	static constexpr int32 NAVMESH_SET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';
	static constexpr int32 NAVMESH_SET_VERSION = 1;

	struct NavMeshSetHeader
	{
		int32 magic;
		int32 version;
		int32 numTiles;
		dtNavMeshParams params;
	};

	struct NavMeshTileHeader
	{
		dtTileRef tileRef;
		int32 dataSize;
	};
}

NavMeshManager::~NavMeshManager()
{
	if (mpNavMeshQuery != nullptr)
	{
		dtFreeNavMeshQuery(mpNavMeshQuery);
		mpNavMeshQuery = nullptr;
	}

	if (mpNavMesh != nullptr)
	{
		dtFreeNavMesh(mpNavMesh);
		mpNavMesh = nullptr;
	}
}

bool NavMeshManager::LoadFromFile(const std::string& filePath)
{
	FILE* fp = nullptr;
	if (fopen_s(&fp, filePath.c_str(), "rb") != 0 || fp == nullptr)
	{
		NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to open file: {}", filePath);
		return false;
	}

	// 헤더 읽기
	NavMeshSetHeader header;
	if (fread(&header, sizeof(NavMeshSetHeader), 1, fp) != 1)
	{
		NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to read header");
		fclose(fp);
		return false;
	}

	if (header.magic != NAVMESH_SET_MAGIC || header.version != NAVMESH_SET_VERSION)
	{
		NET_ENGINE_LOG_ERROR("[NavMeshManager] Invalid file format: magic={}, version={}", header.magic, header.version);
		fclose(fp);
		return false;
	}

	// NavMesh 생성
	mpNavMesh = dtAllocNavMesh();
	if (mpNavMesh == nullptr)
	{
		NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to allocate dtNavMesh");
		fclose(fp);
		return false;
	}

	dtStatus status = mpNavMesh->init(&header.params);
	if (dtStatusFailed(status))
	{
		NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to init dtNavMesh: status={}", status);
		fclose(fp);
		return false;
	}

	// 타일 읽기
	for (int32 i = 0; i < header.numTiles; ++i)
	{
		NavMeshTileHeader tileHeader;
		if (fread(&tileHeader, sizeof(NavMeshTileHeader), 1, fp) != 1)
		{
			NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to read tile header at index {}", i);
			fclose(fp);
			return false;
		}

		if (tileHeader.tileRef == 0 || tileHeader.dataSize == 0)
		{
			continue;
		}

		unsigned char* data = static_cast<unsigned char*>(dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM));
		if (data == nullptr)
		{
			NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to allocate tile data at index {}", i);
			fclose(fp);
			return false;
		}

		if (fread(data, tileHeader.dataSize, 1, fp) != 1)
		{
			dtFree(data);
			NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to read tile data at index {}", i);
			fclose(fp);
			return false;
		}

		mpNavMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, nullptr);
	}

	fclose(fp);

	// NavMeshQuery 생성
	mpNavMeshQuery = dtAllocNavMeshQuery();
	if (mpNavMeshQuery == nullptr)
	{
		NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to allocate dtNavMeshQuery");
		return false;
	}

	status = mpNavMeshQuery->init(mpNavMesh, 2048);
	if (dtStatusFailed(status))
	{
		NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to init dtNavMeshQuery: status={}", status);
		return false;
	}

	NET_ENGINE_LOG_INFO("[NavMeshManager] Loaded NavMesh from {}, tiles: {}", filePath, header.numTiles);
	return true;
}

bool NavMeshManager::IsLoaded() const
{
	return mpNavMesh != nullptr && mpNavMeshQuery != nullptr;
}

bool NavMeshManager::FindPath(const Vector3f& start, const Vector3f& end, Vector<Vector3f>& outWaypoints) const
{
	if (!IsLoaded())
	{
		return false;
	}

	outWaypoints.clear();

	// Recast 좌표: (x, y, z) — UE5와 동일 (Export 시 좌표계 유지)
	const float startPos[3] = { start.mX, start.mY, start.mZ };
	const float endPos[3] = { end.mX, end.mY, end.mZ };
	const float halfExtents[3] = { 200.0f, 200.0f, 200.0f };

	dtQueryFilter filter;
	filter.setIncludeFlags(0xFFFF);
	filter.setExcludeFlags(0);

	dtPolyRef startRef = 0;
	dtPolyRef endRef = 0;
	float nearestStart[3];
	float nearestEnd[3];

	mpNavMeshQuery->findNearestPoly(startPos, halfExtents, &filter, &startRef, nearestStart);
	mpNavMeshQuery->findNearestPoly(endPos, halfExtents, &filter, &endRef, nearestEnd);

	if (startRef == 0 || endRef == 0)
	{
		return false;
	}

	// 폴리곤 경로 탐색
	dtPolyRef polys[MAX_PATH_POLYS];
	int32 polyCount = 0;

	dtStatus status = mpNavMeshQuery->findPath(startRef, endRef, nearestStart, nearestEnd, &filter, polys, &polyCount, MAX_PATH_POLYS);
	if (dtStatusFailed(status) || polyCount == 0)
	{
		return false;
	}

	// 직선 경로로 단순화
	float straightPath[MAX_STRAIGHT_PATH * 3];
	unsigned char straightPathFlags[MAX_STRAIGHT_PATH];
	dtPolyRef straightPathPolys[MAX_STRAIGHT_PATH];
	int32 straightPathCount = 0;

	status = mpNavMeshQuery->findStraightPath(nearestStart, nearestEnd, polys, polyCount,
		straightPath, straightPathFlags, straightPathPolys, &straightPathCount, MAX_STRAIGHT_PATH);

	if (dtStatusFailed(status) || straightPathCount == 0)
	{
		return false;
	}

	outWaypoints.reserve(straightPathCount);
	for (int32 i = 0; i < straightPathCount; ++i)
	{
		outWaypoints.emplace_back(
			straightPath[i * 3 + 0],
			straightPath[i * 3 + 1],
			straightPath[i * 3 + 2]);
	}

	return true;
}

bool NavMeshManager::IsPositionWalkable(const float x, const float y, const float z, const float tolerance) const
{
	if (!IsLoaded())
	{
		return true; // NavMesh 미로드 시 허용
	}

	const float pos[3] = { x, y, z };
	const float halfExtents[3] = { tolerance, tolerance, tolerance };

	dtQueryFilter filter;
	filter.setIncludeFlags(0xFFFF);
	filter.setExcludeFlags(0);

	dtPolyRef nearestRef = 0;
	float nearestPos[3];

	mpNavMeshQuery->findNearestPoly(pos, halfExtents, &filter, &nearestRef, nearestPos);

	return nearestRef != 0;
}

bool NavMeshManager::GetNearestValidPosition(const float x, const float y, const float z, const float searchRadius, Vector3f& outPosition) const
{
	if (!IsLoaded())
	{
		return false;
	}

	const float pos[3] = { x, y, z };
	const float halfExtents[3] = { searchRadius, searchRadius, searchRadius };

	dtQueryFilter filter;
	filter.setIncludeFlags(0xFFFF);
	filter.setExcludeFlags(0);

	dtPolyRef nearestRef = 0;
	float nearestPos[3];

	mpNavMeshQuery->findNearestPoly(pos, halfExtents, &filter, &nearestRef, nearestPos);

	if (nearestRef == 0)
	{
		return false;
	}

	outPosition.mX = nearestPos[0];
	outPosition.mY = nearestPos[1];
	outPosition.mZ = nearestPos[2];
	return true;
}
