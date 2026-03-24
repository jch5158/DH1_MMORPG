# Session ID System Design

## Overview

엔진 레이어에 Redis 기반 글로벌 유니크 Session ID 시스템을 추가한다.
서버 간 통신 시 세션을 포인터가 아닌 ID로 식별하기 위함이다.

## ID 발급 방식

- Redis `INCRBY` 로 서버별 블록(1000개) 단위 예약
- 로컬 `atomic<uint64>` 카운터로 블록 범위 내에서 발급
- 블록 소진 시 동기적으로 새 블록 할당
- 향후 임계치(80%) 도달 시 TimingWheel 기반 백그라운드 선할당으로 개선 예정

```
서버 시작:   Redis INCRBY session:id:counter 1000 → [1, 1000] 확보
로컬 발급:   1, 2, 3, ... 1000
소진 시:     Redis INCRBY session:id:counter 1000 → [1001, 2000] 확보
```

## 신규 클래스

### RedisConnection (엔진 레이어)

단순 동기 Redis 래퍼. SessionIdAllocator 전용.
컨텐츠 레이어의 RedisActor/RedisService와 독립적으로 운영된다.

```
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

```
class SessionIdAllocator
{
public:
    bool Initialize(RedisConnection& redisConnection);
    uint64 Allocate();

private:
    bool allocateBlock();

    static constexpr int64 BLOCK_SIZE = 1000;
    static constexpr const char* REDIS_KEY = "session:id:counter";

    RedisConnection* mpRedisConnection;
    std::atomic<uint64> mNextId;
    uint64 mBlockEnd;
};
```

- `Initialize()`: 첫 블록 할당
- `Allocate()`: 로컬 카운터 증가, 소진 시 `allocateBlock()` 호출
- 향후: `mBlockEnd`의 80% 도달 시 TimingWheel로 선할당 예약

## 기존 클래스 변경

### Session

- `uint64 mSessionId` 멤버 추가 (초기값 0)
- `Initialize()`: `SessionIdAllocator::Allocate()`로 ID 발급
- `Stop()`: `mSessionId = 0` 초기화 (None 상태 복귀)
- `GetSessionId()` public 접근자 추가

### SessionManager

- 내부 자료구조: `Set<SessionRef>` → `HashMap<uint64, SessionRef>`
- `AddSession()`: sessionId를 키로 등록
- `RemoveSession()`: sessionId로 삭제
- `GetSession(uint64 sessionId)` 조회 메서드 추가
- `GetActiveSessions()`, `GetCurrentSessionCount()` 유지

### NetServiceConfig

- `std::string redisConnectionUri` 필드 추가
- `NetService::Initialize()`에서 `RedisConnection` + `SessionIdAllocator` 생성

## 상태 생명주기와 Session ID

```
None (id=0)
  → Initialize() → Disconnected (id=발급됨)
    → Connected → InGame → Disconnecting → Disconnected
      → Stop() → None (id=0)
```

- ID는 Initialize()에서 발급, Stop()에서 초기화
- SessionManager는 AddSession(Connected 시점)에서 등록, RemoveSession(Stop 시점)에서 해제

## 레이어 분리

```
엔진 레이어:    RedisConnection (동기) ← SessionIdAllocator ← Session/SessionManager
컨텐츠 레이어:  RedisActor + RedisService (Actor 기반 비동기) ← 기존 유지, 변경 없음
```

두 Redis 연결은 독립적이며 서로 간섭하지 않는다.
