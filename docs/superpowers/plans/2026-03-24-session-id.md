# Session ID System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Redis 블록 할당 기반 글로벌 유니크 Session ID를 엔진 레이어에 추가한다.

**Architecture:** `RedisConnection`(동기 래퍼) → `SessionIdAllocator`(블록 기반 ID 발급) → `Session`(ID 보유) → `SessionManager`(ID 기반 HashMap 관리). Redis 미설정 시 로컬 atomic 카운터 폴백.

**Tech Stack:** C++, redis-plus-plus (sw::redis), IOCP engine

**Spec:** `docs/superpowers/specs/2026-03-24-session-id-design.md`

---

### Task 1: RedisConnection 클래스 생성

**Files:**
- Create: `DH1_Engine/CppNetEngine/RedisConnection.h`
- Create: `DH1_Engine/CppNetEngine/RedisConnection.cpp`
- Modify: `DH1_Engine/CppNetEngine/CppNetEngine.vcxproj` (ClInclude, ClCompile 추가)

- [ ] **Step 1: RedisConnection.h 작성**

```cpp
#pragma once
#include <sw/redis++/redis++.h>

class RedisConnection
{
public:
	RedisConnection(const RedisConnection&) = delete;
	RedisConnection& operator=(const RedisConnection&) = delete;
	RedisConnection(RedisConnection&&) = delete;
	RedisConnection& operator=(RedisConnection&&) = delete;

	explicit RedisConnection();
	~RedisConnection() = default;

	bool Initialize(const std::string& connectionUri);
	int64 IncrBy(const std::string& key, int64 amount);

private:
	std::unique_ptr<sw::redis::Redis> mRedis;
};
```

- [ ] **Step 2: RedisConnection.cpp 작성**

```cpp
#include "pch.h"
#include "RedisConnection.h"

RedisConnection::RedisConnection()
	: mRedis()
{
}

bool RedisConnection::Initialize(const std::string& connectionUri)
{
	try
	{
		const sw::redis::Uri uri(connectionUri);
		sw::redis::ConnectionOptions options = uri.connection_options();
		sw::redis::ConnectionPoolOptions poolOptions = uri.connection_pool_options();

		mRedis = std::make_unique<sw::redis::Redis>(options, poolOptions);

		const std::string pingReply = mRedis->ping();
		NET_ENGINE_LOG_INFO("[RedisConnection] Initialized. Ping reply : {}", pingReply);
		return true;
	}
	catch (const sw::redis::Error& e)
	{
		NET_ENGINE_LOG_ERROR("[RedisConnection] Initialization Failed : {}", e.what());
		return false;
	}
}

int64 RedisConnection::IncrBy(const std::string& key, int64 amount)
{
	if (!mRedis)
	{
		return 0;
	}

	try
	{
		const int64 result = mRedis->incrby(key, amount);
		return result;
	}
	catch (const sw::redis::Error& e)
	{
		NET_ENGINE_LOG_ERROR("[RedisConnection] IncrBy Failed : {}", e.what());
		return 0;
	}
}
```

- [ ] **Step 3: vcxproj에 파일 추가**

`CppNetEngine.vcxproj`의 `<ClInclude>` 섹션에 `<ClInclude Include="RedisConnection.h" />` 추가.
`<ClCompile>` 섹션에 `<ClCompile Include="RedisConnection.cpp" />` 추가.

- [ ] **Step 4: 커밋**

```bash
git add DH1_Engine/CppNetEngine/RedisConnection.h DH1_Engine/CppNetEngine/RedisConnection.cpp DH1_Engine/CppNetEngine/CppNetEngine.vcxproj
git commit -m "Add RedisConnection synchronous wrapper for engine layer"
```

---

### Task 2: SessionIdAllocator 클래스 생성

**Files:**
- Create: `DH1_Engine/CppNetEngine/SessionIdAllocator.h`
- Create: `DH1_Engine/CppNetEngine/SessionIdAllocator.cpp`
- Modify: `DH1_Engine/CppNetEngine/CppNetEngine.vcxproj`

- [ ] **Step 1: SessionIdAllocator.h 작성**

```cpp
#pragma once

class RedisConnection;

class SessionIdAllocator
{
public:
	SessionIdAllocator(const SessionIdAllocator&) = delete;
	SessionIdAllocator& operator=(const SessionIdAllocator&) = delete;
	SessionIdAllocator(SessionIdAllocator&&) = delete;
	SessionIdAllocator& operator=(SessionIdAllocator&&) = delete;

	explicit SessionIdAllocator();
	~SessionIdAllocator() = default;

	bool Initialize(RedisConnection* pRedisConnection);
	uint64 Allocate();

private:
	bool allocateBlock();

	static constexpr int64 BLOCK_SIZE = 1000;
	static constexpr const char* SESSION_ID_REDIS_KEY = "session:id:counter";

	RedisConnection* mpRedisConnection;
	Mutex mBlockLock;
	uint64 mNextId;
	uint64 mBlockEnd;
};
```

- [ ] **Step 2: SessionIdAllocator.cpp 작성**

```cpp
#include "pch.h"
#include "SessionIdAllocator.h"
#include "RedisConnection.h"

SessionIdAllocator::SessionIdAllocator()
	: mpRedisConnection(nullptr)
	, mBlockLock()
	, mNextId(0)
	, mBlockEnd(0)
{
}

bool SessionIdAllocator::Initialize(RedisConnection* pRedisConnection)
{
	mpRedisConnection = pRedisConnection;
	return allocateBlock();
}

uint64 SessionIdAllocator::Allocate()
{
	while (true)
	{
		const uint64 id = mNextId++;
		if (id <= mBlockEnd)
		{
			return id;
		}

		UniqueLock lock(mBlockLock);

		if (mNextId <= mBlockEnd)
		{
			continue;
		}

		if (!allocateBlock())
		{
			return 0;
		}
	}
}

bool SessionIdAllocator::allocateBlock()
{
	if (mpRedisConnection == nullptr)
	{
		static std::atomic<uint64> sLocalCounter(0);
		mNextId = sLocalCounter.fetch_add(BLOCK_SIZE) + 1;
		mBlockEnd = mNextId + BLOCK_SIZE - 1;
		return true;
	}

	const int64 blockEnd = mpRedisConnection->IncrBy(SESSION_ID_REDIS_KEY, BLOCK_SIZE);
	if (blockEnd == 0)
	{
		NET_ENGINE_LOG_ERROR("[SessionIdAllocator] Failed to allocate block from Redis");
		return false;
	}

	mNextId = static_cast<uint64>(blockEnd - BLOCK_SIZE + 1);
	mBlockEnd = static_cast<uint64>(blockEnd);

	NET_ENGINE_LOG_INFO("[SessionIdAllocator] Block allocated: [{}, {}]", mNextId, mBlockEnd);
	return true;
}
```

- [ ] **Step 3: vcxproj에 파일 추가**

`CppNetEngine.vcxproj`의 `<ClInclude>` 섹션에 `<ClInclude Include="SessionIdAllocator.h" />` 추가.
`<ClCompile>` 섹션에 `<ClCompile Include="SessionIdAllocator.cpp" />` 추가.

- [ ] **Step 4: 커밋**

```bash
git add DH1_Engine/CppNetEngine/SessionIdAllocator.h DH1_Engine/CppNetEngine/SessionIdAllocator.cpp DH1_Engine/CppNetEngine/CppNetEngine.vcxproj
git commit -m "Add SessionIdAllocator with Redis block allocation"
```

---

### Task 3: Session에 sessionId 멤버 추가

**Files:**
- Modify: `DH1_Engine/CppNetEngine/Session.h`
- Modify: `DH1_Engine/CppNetEngine/Session.cpp`

- [ ] **Step 1: Session.h에 mSessionId 멤버 및 접근자 추가**

`public` 섹션에 추가:
```cpp
[[nodiscard]] uint64 GetSessionId() const;
```

`private` 멤버 섹션(`NetServiceRef mpService;` 위)에 추가:
```cpp
uint64 mSessionId;
```

- [ ] **Step 2: Session.cpp 수정**

생성자 초기화 리스트에 `mSessionId(0)` 추가 (`mpService()` 앞에):
```cpp
Session::Session(const int32 receiveBufferSize)
	: SocketIocpObject()
	, mSessionId(0)
	, mpService()
	...
```

`GetSessionId()` 구현 추가 (`GetSessionRef()` 뒤에):
```cpp
uint64 Session::GetSessionId() const
{
	return mSessionId;
}
```

`Initialize()` 함수에서 `setSessionInitialized()` 성공 후, `CreateSocket()` 전에 ID 발급 추가:
```cpp
bool Session::Initialize(const NetServiceRef& pService)
{
	if (!setSessionInitialized())
	{
		return false;
	}

	mSessionId = pService->AllocateSessionId();
	if (mSessionId == 0)
	{
		return false;
	}

	if (CreateSocket() == false)
	...
```

`Stop()` 함수에서 `setSessionNone()` 앞에 ID 초기화 추가:
```cpp
void Session::Stop()
{
	if (!IsDisconnected())
	{
		return;
	}

	OnDisconnected();

	mSessionId = 0;
	setSessionNone();
}
```

- [ ] **Step 3: 커밋**

```bash
git add DH1_Engine/CppNetEngine/Session.h DH1_Engine/CppNetEngine/Session.cpp
git commit -m "Add sessionId to Session with allocation on Initialize and reset on Stop"
```

---

### Task 4: NetService에 SessionIdAllocator 통합

**Files:**
- Modify: `DH1_Engine/CppNetEngine/NetService.h`
- Modify: `DH1_Engine/CppNetEngine/NetService.cpp`
- Modify: `DH1_Engine/CppNetEngine/SharedPtrTypes.h`
- Modify: `DH1_Engine/CppNetEngine/EngineNetwork.h`

- [ ] **Step 1: SharedPtrTypes.h에 타입 선언 추가**

`DECLARE_SMART_PTR(SessionReaper);` 위에 추가:
```cpp
DECLARE_SHARED_PTR(RedisConnection);
DECLARE_SHARED_PTR(SessionIdAllocator);
```

- [ ] **Step 2: EngineNetwork.h에 include 추가**

`#include "SessionManager.h"` 뒤에 추가:
```cpp
#include "RedisConnection.h"
#include "SessionIdAllocator.h"
```

- [ ] **Step 3: NetServiceConfig에 redisConnectionUri 추가**

`NetService.h`의 `NetServiceConfig` 구조체에 추가:
```cpp
struct NetServiceConfig
{
	virtual ~NetServiceConfig() = default;

	int32 maxSessionCount;
	NetAddress netAddress;
	NetworkSchedulerRef pNetworkScheduler;
	SessionFactory sessionFactory;
	std::string redisConnectionUri;
};
```

- [ ] **Step 4: NetService 클래스에 멤버 및 메서드 추가**

`NetService.h`의 `public` 섹션에 추가:
```cpp
uint64 AllocateSessionId();
```

`protected` 섹션에 추가 (`SessionManager mSessionManager;` 뒤에):
```cpp
RedisConnectionRef mpRedisConnection;
SessionIdAllocatorRef mpSessionIdAllocator;
```

- [ ] **Step 5: NetService.cpp의 Initialize에서 allocator 초기화**

`NetService::Initialize()` 함수 끝, `return true;` 전에 추가:
```cpp
mpSessionIdAllocator = cpp_net_engine::MakeShared<SessionIdAllocator>();

if (!config.redisConnectionUri.empty())
{
	mpRedisConnection = cpp_net_engine::MakeShared<RedisConnection>();
	if (!mpRedisConnection->Initialize(config.redisConnectionUri))
	{
		return false;
	}
}

if (!mpSessionIdAllocator->Initialize(mpRedisConnection.get()))
{
	return false;
}
```

`AllocateSessionId()` 구현 추가:
```cpp
uint64 NetService::AllocateSessionId()
{
	if (mpSessionIdAllocator == nullptr)
	{
		return 0;
	}

	return mpSessionIdAllocator->Allocate();
}
```

NetService 생성자 초기화 리스트에 추가:
```cpp
, mpRedisConnection()
, mpSessionIdAllocator()
```

- [ ] **Step 6: 커밋**

```bash
git add DH1_Engine/CppNetEngine/NetService.h DH1_Engine/CppNetEngine/NetService.cpp DH1_Engine/CppNetEngine/SharedPtrTypes.h DH1_Engine/CppNetEngine/EngineNetwork.h
git commit -m "Integrate SessionIdAllocator into NetService with Redis config support"
```

---

### Task 5: SessionManager를 HashMap으로 변경

**Files:**
- Modify: `DH1_Engine/CppNetEngine/SessionManager.h`
- Modify: `DH1_Engine/CppNetEngine/SessionManager.cpp`

- [ ] **Step 1: SessionManager.h 변경**

`Set<SessionRef> mSessions;` → `HashMap<uint64, SessionRef> mSessions;` 변경.

`GetSession` 메서드 추가:
```cpp
[[nodiscard]] SessionRef GetSession(const uint64 sessionId);
```

- [ ] **Step 2: SessionManager.cpp 변경**

`AddSession` 변경 — sessionId를 키로 사용:
```cpp
bool SessionManager::AddSession(SessionRef pSession)
{
	UniqueLock lock(mLock);

	if (std::cmp_less_equal(mMaxSessionCount, mSessions.size()))
	{
		return false;
	}

	const uint64 sessionId = pSession->GetSessionId();
	const bool retVal = mSessions.try_emplace(sessionId, std::move(pSession)).second;
	return retVal;
}
```

`RemoveSession` 변경 — sessionId로 삭제:
```cpp
void SessionManager::RemoveSession(const SessionRef& pSession)
{
	UniqueLock lock(mLock);
	mSessions.erase(pSession->GetSessionId());
}
```

`GetFirstSessionRef` 변경 — HashMap iterator 대응:
```cpp
SessionRef SessionManager::GetFirstSessionRef()
{
	UniqueLock lock(mLock);
	if (mSessions.begin() == mSessions.end())
	{
		return nullptr;
	}

	return mSessions.begin()->second;
}
```

`GetActiveSessions` 변경 — value만 추출:
```cpp
Vector<SessionRef> SessionManager::GetActiveSessions()
{
	SharedLock lock(mLock);

	Vector<SessionRef> sessions;
	sessions.reserve(mSessions.size());
	for (const auto& [sessionId, pSession] : mSessions)
	{
		sessions.emplace_back(pSession);
	}

	return sessions;
}
```

`GetSession` 구현 추가:
```cpp
SessionRef SessionManager::GetSession(const uint64 sessionId)
{
	SharedLock lock(mLock);
	const auto iter = mSessions.find(sessionId);
	if (iter == mSessions.end())
	{
		return nullptr;
	}

	return iter->second;
}
```

- [ ] **Step 3: 커밋**

```bash
git add DH1_Engine/CppNetEngine/SessionManager.h DH1_Engine/CppNetEngine/SessionManager.cpp
git commit -m "Change SessionManager from Set to HashMap keyed by sessionId"
```

---

### Task 6: GatewayServer에 Redis URI 설정 추가

**Files:**
- Modify: `DH1_Server/GatewayServer/GatewayService.cpp`
- Modify: `Shared/Config/Server/GatewayServer.json` (redis connectionUri가 이미 있으므로 확인만)

- [ ] **Step 1: GatewayService.cpp에서 redisConnectionUri 설정**

`GatewayService::Initialize()`에서 `serviceConfig` 설정 부분에 추가 (`serviceConfig.sessionFactory = ...` 뒤):
```cpp
serviceConfig.redisConnectionUri = redisConfig.GetString("connectionUri");
```

- [ ] **Step 2: 커밋**

```bash
git add DH1_Server/GatewayServer/GatewayService.cpp
git commit -m "Pass Redis connection URI to ServerServiceConfig for session ID allocation"
```

---

### Task 7: 빌드 검증

- [ ] **Step 1: 솔루션 전체 빌드**

Visual Studio 또는 MSBuild로 `DH1_Engine/DH1_Engine.slnx` 빌드. 에러 없이 컴파일되는지 확인.

- [ ] **Step 2: GatewayServer 빌드**

`DH1_Server/GatewayServer/` 프로젝트 빌드. Redis URI가 정상 전달되는지 확인.

- [ ] **Step 3: 최종 커밋 및 푸시**

```bash
git push origin main
```
