#pragma once

struct dtNavMesh;
struct dtNavMeshQuery;

struct Vector3f
{
	float mX = 0.0f;
	float mY = 0.0f;
	float mZ = 0.0f;

	Vector3f() = default;
	Vector3f(const float x, const float y, const float z) : mX(x), mY(y), mZ(z) {}
};

class NavMeshManager
{
public:

	NavMeshManager() = default;
	~NavMeshManager();

	NavMeshManager(const NavMeshManager&) = delete;
	NavMeshManager& operator=(const NavMeshManager&) = delete;
	NavMeshManager(NavMeshManager&&) = delete;
	NavMeshManager& operator=(NavMeshManager&&) = delete;

	[[nodiscard]] bool LoadFromFile(const std::string& filePath);
	[[nodiscard]] bool IsLoaded() const;

	// 경로 탐색: start → end, 결과를 waypoints에 저장. 성공 시 true
	[[nodiscard]] bool FindPath(const Vector3f& start, const Vector3f& end, Vector<Vector3f>& outWaypoints) const;

	// 위치가 NavMesh 위에 있는지 확인
	[[nodiscard]] bool IsPositionWalkable(float x, float y, float z, float tolerance = 200.0f) const;

	// 가장 가까운 유효한 NavMesh 위치로 스냅
	[[nodiscard]] bool GetNearestValidPosition(float x, float y, float z, float searchRadius, Vector3f& outPosition) const;

private:

	dtNavMesh* mpNavMesh = nullptr;
	dtNavMeshQuery* mpNavMeshQuery = nullptr;

	static constexpr int32 MAX_PATH_POLYS = 256;
	static constexpr int32 MAX_STRAIGHT_PATH = 256;
};
