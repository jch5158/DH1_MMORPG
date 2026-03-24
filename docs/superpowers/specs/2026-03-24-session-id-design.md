# Session ID System Design

## Overview

엔진 레이어에 Redis 기반 글로벌 유니크 Session ID 시스템을 추가한다.
서버 간 통신 시 세션을 포인터가 아닌 ID로 식별하기 위함이다.

## ID 발급 방식

- Redis `INCRBY` 로 서버별 블록(1000개) 단위 예약
- 로컬 `atomic<uint64>` 카운터로 블록 범위 내에서 발급
- 블록 소진 시 동기적으로 새 블록 할당
- 향후 임계치(80%) 도달 시 TimingWheel 기반 백그라운드 선할당으로 개선 예정
- Redis 미설정 시(EchoServer 등) 로컬 `atomic<uint64>` 카운터 폴백

```
서버 시작:   Redis INCRBY session:id:counter 1000 → 반환값 1000
             블록 범위: [1001 - 1000, 1000] = [1, 1000]
로컬 발급:   1, 2, 3, ... 1000
소진 시:     Redis INCRBY session:id:counter 1000 → 반환값 2000
             블록 범위: [2001 - 1000, 2000] = [1001, 2000]
```

ID는 서버 재시작 시 리셋되지 않고 Redis 카운터에서 이어서 발급된다.
uint64 범위(18경)이므로 오버플로우 걱정은 불필요하다.

## 신규 클래스

### RedisConnection (엔진 레이어)

단순 동기 Redis 래퍼. SessionIdAllocator 전용.
컨텐츠 레이어의 RedisActor/RedisService와 독립적으로 운영된다.

```cpp
class RedisConnection
{
public:
    bool Initialize(const std::string& connectionUri);
    int64 IncrBy(const std::string& key, int64 amount);

private:
    std::unique_ptr<sw::redis::Redis> mRedis;
};
```

### SessionIdAllocator (엔진 레이어)

Redis 블록 할당 기반 ID 발급기.

```cpp
class SessionIdAllocator
{
public:
    bool Initialize(RedisConnection& redisConnection);
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

- `Initialize()`: 첫 블록 할당. Redis 실패 시 false 반환 → 서비스 시작 실패 (fail-fast).
- `Allocate()`: 로컬 카운터 증가 (lock-free). 블록 소진 시 `mBlockLock` 획득 후 `allocateBlock()` 호출. Redis 실패 시 0 반환 → `Session::Initialize()` 실패.
- 향후: `mBlockEnd`의 80% 도달 시 TimingWheel로 선할당 예약

#### 스레드 안전성

블록 범위 내 발급은 `atomic<uint64>` fetch_add로 lock-free.
블록 소진 시에만 `mBlockLock` (Mutex)을 획득하여 단일 스레드만 `allocateBlock()`을 실행.
다른 스레드는 mutex 대기 후 이미 할당된 새 블록에서 발급.

## 기존 클래스 변경

### Session

- `uint64 mSessionId` 멤버 추가 (초기값 0)
- `Initialize()`: `SessionIdAllocator::Allocate()`로 ID 발급. 0 반환 시 Initialize 실패.
- `Stop()`: `mSessionId = 0` 초기화 (None 상태 복귀)
- `GetSessionId()` public 접근자 추가

### SessionManager

- 내부 자료구조: `Set<SessionRef>` → `HashMap<uint64, SessionRef>`
- `AddSession(SessionRef pSession)`: `pSession->GetSessionId()`를 키로 등록
- `RemoveSession(const SessionRef& pSession)`: `pSession->GetSessionId()`로 삭제. 시그니처 변경 없음.
- `GetSession(uint64 sessionId)` 조회 메서드 추가
- `GetActiveSessions()`, `GetCurrentSessionCount()`, `GetFirstSessionRef()` 유지

### NetServiceConfig

- `std::string redisConnectionUri` 필드 추가 (기본값: 빈 문자열)
- 빈 문자열이면 Redis 미사용, 로컬 atomic 카운터 폴백 (EchoServer 호환)

### NetService

- `RedisConnectionRef mpRedisConnection` 멤버 추가
- `SessionIdAllocatorRef mpSessionIdAllocator` 멤버 추가
- `Initialize()`: redisConnectionUri가 비어있지 않으면 RedisConnection + SessionIdAllocator 생성, 비어있으면 로컬 폴백 Allocator 생성
- `Session::Initialize()`가 `NetServiceRef`를 통해 allocator에 접근

## 상태 생명주기와 Session ID

```
None (id=0)
  → Initialize() → Disconnected (id=발급됨)
    → Connected → InGame → Disconnecting → Disconnected
      → Stop() → None (id=0)
```

- Disconnected 상태가 두 번 등장: 초기화 직후 / Disconnect 완료 후. 둘 다 동일한 eSessionState::Disconnected.
- 두 경우 모두 같은 sessionId를 유지. ID 변경은 Stop() → Initialize() 사이클에서만 발생.
- ID는 Initialize()에서 발급, Stop()에서 0으로 초기화.
- SessionManager는 AddSession(Connected 시점)에서 등록, RemoveSession(Stop 시점)에서 해제.

## 에러 처리

| 시점 | 실패 원인 | 동작 |
|------|-----------|------|
| 서버 시작 | Redis 연결 실패 | Initialize() false → 서비스 시작 중단 (fail-fast) |
| 런타임 블록 할당 | Redis 일시 장애 | Allocate() → 0 반환 → Session::Initialize() 실패 → CreateSession() nullptr |
| Redis 미설정 | redisConnectionUri 빈 문자열 | 로컬 atomic 카운터 폴백 (글로벌 유니크 미보장, 단일 서버용) |

## 레이어 분리

```
엔진 레이어:    RedisConnection (동기) ← SessionIdAllocator ← Session/SessionManager
컨텐츠 레이어:  RedisActor + RedisService (Actor 기반 비동기) ← 기존 유지, 변경 없음
```

두 Redis 연결은 독립적이며 서로 간섭하지 않는다.
