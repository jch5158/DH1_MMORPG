#include "pch.h"
#include "NavMeshManager.h"
// UE5 Detour 사용 (vcpkg standard Detour와 포맷 불일치로 대체)
#include "Detour/DetourNavMesh.h"
#include "Detour/DetourNavMeshQuery.h"

// NavMesh Export 파일 헤더
// UE5 Exporter는 표준 Detour dtNavMeshParams (28 bytes, float) 형식으로 기록하고,
// dtTileRef도 uint32 (4 bytes) 로 기록한다. (tile data 내부만 UE5 포맷)
namespace
{
	static constexpr int32 NAVMESH_SET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';
	static constexpr int32 NAVMESH_SET_VERSION = 1;

	// 파일에 실제로 저장된 표준 Detour 파라미터 (float, 28 bytes)
	struct FileNavMeshParams
	{
		float orig[3];
		float tileWidth;
		float tileHeight;
		int32 maxTiles;
		int32 maxPolys;
	};

	struct NavMeshSetHeader
	{
		int32 magic;
		int32 version;
		int32 numTiles;
		FileNavMeshParams params;  // 표준 28-byte 포맷
	};

	struct NavMeshTileHeader
	{
		uint32 tileRef;   // 파일에는 uint32 (4 bytes), UE5 dtTileRef(uint64)이 아님
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

	// 파일의 표준 28-byte 파라미터를 UE5 확장 dtNavMeshParams로 변환
	// UE5 dtNavMeshParams는 walkableHeight/Radius/Climb, resolutionParams[3].bvQuantFactor 필드를 추가로 가짐
	// bvQuantFactor: 폴리곤 BV트리 쿼리에 사용. 빌드 시 1/cellSize로 설정됨 (cs≈10cm → 0.1f)
	dtNavMeshParams ue5Params = {};
	ue5Params.orig[0]      = static_cast<dtReal>(header.params.orig[0]);
	ue5Params.orig[1]      = static_cast<dtReal>(header.params.orig[1]);
	ue5Params.orig[2]      = static_cast<dtReal>(header.params.orig[2]);
	ue5Params.tileWidth    = static_cast<dtReal>(header.params.tileWidth);
	ue5Params.tileHeight   = static_cast<dtReal>(header.params.tileHeight);
	ue5Params.maxTiles     = header.params.maxTiles;
	ue5Params.maxPolys     = header.params.maxPolys;
	// walkableHeight/Radius/Climb는 쿼리 로직에서 사용되지 않으므로 0으로 유지
	// bvQuantFactor: 보수적으로 큰 값을 사용하면 모든 폴리곤을 검색 (정확하지만 느림)
	// 1.0f / cs (cs=10cm) = 0.1f 사용. 잘못된 경우 폴리곤이 더 많이 검색될 수 있으나 결과는 정확함
	constexpr dtReal kBvQuantFactor = static_cast<dtReal>(1.0 / 19.0);
	for (int32 r = 0; r < DT_RESOLUTION_COUNT; ++r)
	{
		ue5Params.resolutionParams[r].bvQuantFactor = kBvQuantFactor;
	}

	dtStatus status = mpNavMesh->init(&ue5Params);
	if (dtStatusFailed(status))
	{
		NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to init dtNavMesh: status={}", status);
		fclose(fp);
		return false;
	}

	NET_ENGINE_LOG_INFO("[NavMesh] params: orig=({:.2f},{:.2f},{:.2f}), tileWidth={:.2f}, tileHeight={:.2f}, maxTiles={}, maxPolys={}",
		header.params.orig[0],
		header.params.orig[1],
		header.params.orig[2],
		header.params.tileWidth,
		header.params.tileHeight,
		header.params.maxTiles,
		header.params.maxPolys);

	// 타일 읽기
	int32 addedTiles = 0;
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
			// tileRef=0이지만 dataSize>0이면 파일 포인터를 data 크기만큼 건너뜀
			if (tileHeader.dataSize > 0)
			{
				fseek(fp, tileHeader.dataSize, SEEK_CUR);
			}
			continue;
		}

		unsigned char* data = static_cast<unsigned char*>(dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM_TILE_DATA));
		if (data == nullptr)
		{
			NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to allocate tile data at index {}", i);
			fclose(fp);
			return false;
		}

		if (fread(data, tileHeader.dataSize, 1, fp) != 1)
		{
			dtFree(data, DT_ALLOC_PERM_TILE_DATA);
			NET_ENGINE_LOG_ERROR("[NavMeshManager] Failed to read tile data at index {}", i);
			fclose(fp);
			return false;
		}

		// lastRef=0: Detour가 빈 슬롯을 자동 할당
		// 파일의 tileRef(1,2,3...)는 순번 값이며 인코딩된 dtTileRef가 아니므로 lastRef로 사용하지 않음
		const dtStatus addStatus = mpNavMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, 0, nullptr);
		if (dtStatusSucceed(addStatus))
		{
			++addedTiles;
		}
		else
		{
			NET_ENGINE_LOG_WARN("[NavMesh] addTile failed at index {}, tileRef={}, dataSize={}, status={}",
				i, static_cast<uint64>(tileHeader.tileRef), tileHeader.dataSize, addStatus);
		}
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

	NET_ENGINE_LOG_INFO("[NavMeshManager] Loaded NavMesh from {}, total entries: {}, added tiles: {}", filePath, header.numTiles, addedTiles);
	return true;
}

bool NavMeshManager::IsLoaded() const
{
	return mpNavMesh != nullptr && mpNavMeshQuery != nullptr;
}

// UE5 좌표 (X, Y, Z) → Recast 내부 좌표 (Y, Z, -X)
// UE5: X=Forward, Y=Right, Z=Up (Left-handed Z-up)
// Recast: X=Right, Y=Up, Z=Backward (Right-handed Y-up)
static void UE5ToRecast(const float ue_x, const float ue_y, const float ue_z, dtReal out[3])
{
	out[0] = static_cast<dtReal>(ue_y);
	out[1] = static_cast<dtReal>(ue_z);
	out[2] = static_cast<dtReal>(-ue_x);
}

// Recast 내부 좌표 (X, Y, Z) → UE5 좌표 (-Z, X, Y)
static Vector3f RecastToUE5(const dtReal rc_x, const dtReal rc_y, const dtReal rc_z)
{
	return Vector3f(
		static_cast<float>(-rc_z),
		static_cast<float>(rc_x),
		static_cast<float>(rc_y));
}

bool NavMeshManager::FindPath(const Vector3f& start, const Vector3f& end, Vector<Vector3f>& outWaypoints) const
{
	if (!IsLoaded())
	{
		return false;
	}

	outWaypoints.clear();

	// UE5 좌표 → Recast 좌표 변환
	dtReal startPos[3];
	dtReal endPos[3];
	UE5ToRecast(start.mX, start.mY, start.mZ, startPos);
	UE5ToRecast(end.mX, end.mY, end.mZ, endPos);
	const dtReal halfExtents[3] = { 200.0, 200.0, 200.0 };

	dtQueryFilter filter;
	filter.setIncludeFlags(0xFFFF);
	filter.setExcludeFlags(0);

	dtPolyRef startRef = 0;
	dtPolyRef endRef = 0;
	dtReal nearestStart[3];
	dtReal nearestEnd[3];

	mpNavMeshQuery->findNearestPoly(startPos, halfExtents, &filter, &startRef, nearestStart);
	mpNavMeshQuery->findNearestPoly(endPos, halfExtents, &filter, &endRef, nearestEnd);

	// 목적지가 NavMesh 밖인 경우 → 전체 NavMesh 범위에서 가장 가까운 폴리곤으로 스냅
	// (클릭 위치가 NavMesh 커버리지 밖이어도 경계 지점까지 경로 탐색)
	if (endRef == 0)
	{
		constexpr dtReal kWideExtent = 15000.0;
		const dtReal halfExtentsWide[3] = { kWideExtent, kWideExtent, kWideExtent };
		mpNavMeshQuery->findNearestPoly(endPos, halfExtentsWide, &filter, &endRef, nearestEnd);
	}

	NET_ENGINE_LOG_INFO("[NavMesh] FindPath: startRC=({:.1f},{:.1f},{:.1f}) ref={}, endRC=({:.1f},{:.1f},{:.1f}) ref={}",
		static_cast<float>(startPos[0]), static_cast<float>(startPos[1]), static_cast<float>(startPos[2]),
		static_cast<uint64>(startRef),
		static_cast<float>(nearestEnd[0]), static_cast<float>(nearestEnd[1]), static_cast<float>(nearestEnd[2]),
		static_cast<uint64>(endRef));

	if (startRef == 0 || endRef == 0)
	{
		return false;
	}

	// 폴리곤 경로 탐색 (UE5 API: dtQueryResult)
	dtQueryResult pathResult;
	dtStatus status = mpNavMeshQuery->findPath(startRef, endRef, nearestStart, nearestEnd,
		DT_REAL_MAX, &filter, pathResult, nullptr);

	if (dtStatusFailed(status) || pathResult.size() == 0)
	{
		return false;
	}

	// 폴리 레퍼런스 추출
	const int32 polyCount = pathResult.size();
	dtPolyRef polys[MAX_PATH_POLYS];
	pathResult.copyRefs(polys, MAX_PATH_POLYS);

	// 직선 경로로 단순화 (UE5 API: dtQueryResult)
	dtQueryResult straightResult;
	status = mpNavMeshQuery->findStraightPath(nearestStart, nearestEnd, polys, polyCount, straightResult);

	if (dtStatusFailed(status) || straightResult.size() == 0)
	{
		return false;
	}

	// Recast 좌표 → UE5 좌표 변환 후 반환
	const int32 straightPathCount = straightResult.size();
	outWaypoints.reserve(straightPathCount);
	for (int32 i = 0; i < straightPathCount; ++i)
	{
		const dtReal* pos = straightResult.getPos(i);
		outWaypoints.emplace_back(RecastToUE5(pos[0], pos[1], pos[2]));
	}

	return true;
}

bool NavMeshManager::IsPositionWalkable(const float x, const float y, const float z, const float tolerance) const
{
	if (!IsLoaded())
	{
		return true; // NavMesh 미로드 시 허용
	}

	dtReal pos[3];
	UE5ToRecast(x, y, z, pos);
	const dtReal halfExtents[3] = { static_cast<dtReal>(tolerance), static_cast<dtReal>(tolerance), static_cast<dtReal>(tolerance) };

	dtQueryFilter filter;
	filter.setIncludeFlags(0xFFFF);
	filter.setExcludeFlags(0);

	dtPolyRef nearestRef = 0;
	dtReal nearestPos[3];

	mpNavMeshQuery->findNearestPoly(pos, halfExtents, &filter, &nearestRef, nearestPos);

	return nearestRef != 0;
}

bool NavMeshManager::GetNearestValidPosition(const float x, const float y, const float z, const float searchRadius, Vector3f& outPosition) const
{
	if (!IsLoaded())
	{
		return false;
	}

	dtReal pos[3];
	UE5ToRecast(x, y, z, pos);
	const dtReal halfExtents[3] = { static_cast<dtReal>(searchRadius), static_cast<dtReal>(searchRadius), static_cast<dtReal>(searchRadius) };

	dtQueryFilter filter;
	filter.setIncludeFlags(0xFFFF);
	filter.setExcludeFlags(0);

	dtPolyRef nearestRef = 0;
	dtReal nearestPos[3];

	mpNavMeshQuery->findNearestPoly(pos, halfExtents, &filter, &nearestRef, nearestPos);

	NET_ENGINE_LOG_INFO("[NavMesh] GetNearestValid: UE5({:.1f},{:.1f},{:.1f}) -> RC({:.1f},{:.1f},{:.1f}), radius={:.1f}, ref={}",
		x, y, z,
		static_cast<float>(pos[0]), static_cast<float>(pos[1]), static_cast<float>(pos[2]),
		searchRadius, static_cast<uint64>(nearestRef));

	if (nearestRef == 0)
	{
		return false;
	}

	// Recast 좌표 → UE5 좌표 변환
	outPosition = RecastToUE5(nearestPos[0], nearestPos[1], nearestPos[2]);
	return true;
}
