# DH1_MMORPG — Game Server Engineer Portfolio 2026

[![GitHub](https://img.shields.io/badge/GitHub-jch5158%2FDH1__MMORPG-181717?logo=github)](https://github.com/jch5158/DH1_MMORPG)
[![YouTube](https://img.shields.io/badge/YouTube-%EC%8B%9C%EC%97%B0%20%EC%98%81%EC%83%81-FF0000?logo=youtube)](https://www.youtube.com/watch?v=_RJqDSDwSnE)

---

## 00 개인 소개 & 경력

### 정찬훈 · JEONG CHAN-HUN

**MMORPG / 온라인 게임 서버 개발 · 고성능 네트워크 엔진 구현 · 라이브 서비스 운영·콘텐츠 개발**

C++ IOCP 기반 비동기 네트워크 엔진을 처음부터 직접 설계·구현한 게임 서버 프로그래머입니다. 조이시티에서 프리스타일2 라이브 서비스와 신작 프리스타일 리부트의 시스템·콘텐츠를 개발하며 운영 안정성과 실무 감각을 길렀고, 개인 프로젝트 DH1_MMORPG에서는 네트워크 엔진·분산 서버·서버 사이드 NavMesh·AOI까지 MMORPG 서버의 핵심 파이프라인을 구현했습니다. "동작하는 코드"를 넘어 **왜 그렇게 동작하는지**를 파고드는 깊이를 강점으로 삼습니다.

| 1Y 10M | 2종 | 1인 |
|:---:|:---:|:---:|
| 현업 경력 · 조이시티 | 라이브·신작 프로젝트 | MMORPG 서버 풀스택 구현 |

| | |
|---|---|
| **Email** | devhun001@gmail.com |
| **Phone** | 010-5158-2378 |
| **GitHub** | [github.com/jch5158/DH1_MMORPG](https://github.com/jch5158/DH1_MMORPG) |

---

### 현업 경력

**서버 프로그래머 (사원) · 조이시티 · 프리스타일 4팀**  
2023.05 – 2025.02 (1년 10개월)

프리스타일2(라이브 서비스)·프리스타일 리부트(신작 개발) 서버 개발 — C++17 / C++11 · MS SQL Server · AWS

- **시스템 개발** — 레드닷 시스템, 일일·주간 미션 시스템, 장비 추가 능력치 부여, 2차 비밀번호 등 설계·구현
- **콘텐츠 개발** — 주사위·가위바위보·계단오르기 미니게임 이벤트, 주간 미션 상점, 12주년 SLG 미니게임 이벤트
- **라이브 운영** — 실시간 버그 이슈 대응, 레거시 코드 리팩토링, DB 테이블 명세서 작성, 가챠 확률 검증 치트

> 라이브 서비스에서 버그 하나가 다수 유저에 미치는 영향을 체감하며 **코드 안정성·가독성을 최우선으로 두는 습관**을 확립했습니다.

---

### 학력

| | |
|---|---|
| **한국공학대학교** | 지식서비스공학 · 2015.03 – 2019.02 졸업 |
| **유한공업고등학교** | 2012 – 2015 졸업 |

### 자격 · 교육

| | |
|---|---|
| **정보처리기사** | 한국산업인력공단 · 2019.08 취득 |
| **프로카데미 16기** | 서버 프로그래머 과정 졸업 — 어셈블리 분석·C/C++·네트워크·OS·멀티스레드·IOCP·MMORPG 서버 구현 |

---

## 목차 (Table of Contents)

| # | 챕터 | 내용 |
|:---:|------|------|
| **00** | [개인 소개 & 경력](#00-개인-소개--경력) | 이름·연락처·조이시티 경력·학력·자격 |
| **01** | [Profile & Positioning](#01-profile--positioning) | 핵심 역량 요약 |
| **02** | [Technical Stack](#02-technical-stack) | 기술 스택 전체 목록 |
| **03** | [Featured Project — DH1_MMORPG](#03-featured-project--dh1_mmorpg) | 대표 프로젝트 개요·분산 아키텍처·성과 |
| **04** | [CppNetEngine — High-Performance Networking](#04-cppnetengine--high-performance-networking) | 이중 IOCP·Actor 모델·Lock-free·TimingWheel |
| **05** | [Memory & Multi-Server Systems](#05-memory--multi-server-systems) | 3계층 메모리 풀·멀티 서버 수평 확장 |
| **06** | [Server Authority — NavMesh · AOI · Codegen](#06-server-authority--navmesh--aoi--codegen) | 서버 권위 경로탐색·시야 동기화·코드 자동화 |

---

## 01 Profile & Positioning

### 핵심 역량 요약

저는 "동작하는 코드"에 멈추지 않고 왜 그렇게 동작하는지를 끝까지 파고드는 게임 서버 프로그래머입니다. 라이브러리를 가져다 쓰는 대신 IOCP 비동기 네트워크 엔진·Lock-free 자료구조·메모리 할당기·타이머 스케줄러를 구현하며 OS·네트워크·동시성의 근본을 체득했고, 그 위에 서버 사이드 NavMesh 경로탐색·AOI·GameTick·인게임 채팅 등 실제 MMORPG 서버의 핵심 시스템을 올렸습니다. 조이시티 라이브 서비스에서 다진 운영 안정성과 콘텐츠·시스템 개발 경험이 이 깊이를 실무로 뒷받침합니다.

### 게임 서버 개발자로서의 강점 (Why this profile fits)

| 엔진을 만드는 깊이 | 서버 권위 게임 로직 | 라이브 서비스 실무 |
|------|------|------|
| IOCP Overlapped I/O · Lock-free 큐 · 3계층 메모리 풀 · 계층형 TimingWheel을 **외부 프레임워크 없이** 구현. 서버가 멈추거나 느려지는 근본 원인을 엔진 레벨에서 진단·튜닝합니다. | 서버 사이드 NavMesh 경로탐색, AOI 기반 시야 동기화, GameTick 경로 추종, 범위별 채팅 릴레이 등 **서버가 게임 상태를 관리하는 권위적 구조**를 설계·구현했습니다. | 프리스타일2·리부트에서 미션·레드닷 시스템과 미니게임 이벤트를 개발하고, 라이브 버그 대응·레거시 리팩토링을 수행하며 **유저에게 닿는 코드의 안정성**을 최우선으로 다뤘습니다. |
| 게임 적용 · 동접 부하·렉·메모리 누수의 근본 대응 | 게임 적용 · 서버 이동 검증·시야 동기화 | 게임 적용 · 게임 콘텐츠·시스템 개발, 라이브 이슈 경험 |

---

## 02 Technical Stack

### 기술 스택

| 분류 | 기술 |
|------|------|
| **Languages** (주력 / 보조) | C++17 · C# · SQL · Assembly (분석) |
| **Game Server** (게임 서버 시스템) | 서버 권위 로직 · 인게임 채팅 · 에이전트 서버 기반 패킷 Relay · 서버 사이드 NavMesh · AOI / GridManager |
| **Networking** (저수준 I/O) | Windows IOCP · SendBufferAllocator · HTTPS / REST · Custom Network Engine |
| **Concurrency** (동시성·자료구조) | Actor Model · ScopedActor · Lock-free Queue/Stack · atomic / CAS · 3-Tier Memory Pool · TimingWheel |
| **Multi-Server** (분산·상태 동기화) | Redis (Session / Pub/Sub) · Gateway 로드밸런싱 · 크로스 서버 채팅 |
| **Backend** (서비스 계층) | ASP.NET Core 10 · EF Core (Migrations) · REST API · Background Service |
| **Client** (클라이언트 통합) | Unreal Engine 5.7 (C++) · Custom Network Engine 이식 |
| **Build / Tooling** (빌드·자동화) | PacketGenerator (.NET) · protoc · MSBuild / UBT · vcpkg · BuildScripts (.bat) |

> 표기된 기술은 모두 실제 코드로 구현·운영해 본 항목이며, 학습 수준에 그친 항목은 포함하지 않았습니다.

---

## 03 Featured Project — DH1_MMORPG

### 대표 프로젝트

C++17 IOCP 커스텀 네트워크 엔진(CppNetEngine)을 설계·구현하고, 이를 4개의 분산 서버와 Unreal Engine 5.7 클라이언트가 동일하게 공유하도록 만든 MMORPG 서버 프로젝트입니다. 네트워크 엔진의 내부 컴포넌트(I/O 모델·동시성·메모리·타이머)부터 분산 아키텍처, 클라이언트 통합까지 **서버 개발의 전 과정을 1인이 구현**했습니다.

| | |
|---|---|
| **유형 / 기간** | 개인 프로젝트 (1인 개발) · 2026.01 ~ 진행 중 |
| **구성** | C++17 네트워크 엔진 · C# 인증 서버 · Protobuf 프로토콜 · UE5 클라이언트 — 1인 전 영역 구현 |
| **저장소** | [github.com/jch5158/DH1_MMORPG](https://github.com/jch5158/DH1_MMORPG) |
| **시연 영상** | [youtube.com/watch?v=_RJqDSDwSnE](https://www.youtube.com/watch?v=_RJqDSDwSnE) |

| 2 | 3계층 | 3레벨 | 4 |
|:---:|:---:|:---:|:---:|
| 독립 IOCP 스케줄러<br>(Network / Actor) | 메모리 풀<br>TLS → 글로벌 → OS | 계층형 TimingWheel<br>256·256·128 슬롯 | 분산 서버 역할 분리<br>+ UE5 클라이언트 |

### 시스템 아키텍처 (Distributed Topology)

```
DH1_Client (UE5)
CppNetEngine.lib 이식 · 서버와 동일 엔진 공유
       │
       ├─ TCP ──────────────────────────────────┐
       │                                        ▼
       └─ HTTPS ──────────────►  LoginServer (C# ASP.NET Core 10)
                                  계정 인증 · 세션 티켓
                                  GatewayServer (C++ IOCP)
                              접속 관문 · 인증·라우팅·하트비트
                                        │ Relay (TCP)
                                        ▼
                                  WorldServer (C++ IOCP)
                       게임 로직 · NavMesh 경로탐색 · AOI · GameTick · 채팅 라우팅
                                        │
                                        ▼
                                  RealmServer (C++ IOCP)
                              서버 목록 / 월드 등록 · 상태 조율
                                        │
            ┌───────────────────────────┼──────────────────────┐
            ▼                           ▼                      ▼
        MySQL                         Redis                 AWS S3
    계정·캐릭터 영속             세션·Pub/Sub·서버 레지스트리    NavMesh 바이너리
```

각 C++ 서버는 CppNetEngine을 static lib로 링크하며, 클라이언트(UE5)도 동일 엔진을 이식하여 **서버·클라이언트가 단일 네트워크 스택을 공유**합니다.

### 주요 기여 & 성과

| 항목 | 내용 |
|------|------|
| **커스텀 네트워크 엔진** | IOCP 비동기 I/O · Actor 모델 · Lock-free 자료구조 · 3계층 메모리 풀 · TimingWheel을 포함한 C++ 서버 엔진을 외부 프레임워크 없이 처음부터 구현 |
| **분산 서버 아키텍처** | 역할별 4개 서버 분리 + Relay 패턴 + Redis 세션으로 멀티 서버 구조 구현. LoginServer는 Gateway별 세션 수(Redis 조회)를 비교해 최소 연결 노드로 분배 |
| **엔진 클라이언트 이식** | CppNetEngine을 static lib로 빌드해 UE5에 통합. 매크로 충돌 격리·생명주기 통합(Subsystem)으로 서버·클라 동일 엔진 공유 |
| **프로토콜 자동화 도구** | .proto 커스텀 옵션 기반 PacketGenerator(.NET)를 직접 개발해 역할별 패킷 코드·디스패처를 자동 생성, 서버·클라 프로토콜을 동기화 |

---

## 04 CppNetEngine — High-Performance Networking

### 고성능 네트워크 엔진

엔진의 핵심은 소켓 I/O와 게임 로직을 **물리적으로 분리한 이중 IOCP 스케줄러**입니다. 각 스케줄러는 독립된 IOCP 핸들과 스레드 풀을 가져 서로 간섭하지 않으며, 메시지는 Lock-free 큐로만 전달됩니다.

| 스케줄러 | 역할 | 설명 |
|----------|------|------|
| **NetworkScheduler** | 소켓 I/O 전담 | Overlapped I/O 완료를 GetQueuedCompletionStatus로 수신 → 패킷 역직렬화·송신 완료 처리. 디스패치 루프마다 TimingWheel을 Tick하여 하트비트·세션 타임아웃을 함께 스케줄링 |
| **ActorScheduler** | 게임 로직 전담 | 소켓 없이 PostQueuedCompletionStatus로 Actor를 깨우는 수동 완료 큐. 워크 아이템을 IOCP 동시성 제어에 그대로 위임하여 별도 락 없이 스레드 부하를 분산 |

### Actor 모델 — 락 없는 단일 소유 동시성

각 Actor는 단일 `atomic<bool>` 플래그의 `TryAcquire / Release`로 보호되어, **한 번에 하나의 디스패치 스레드만 진입**합니다. 멀티 객체를 동시에 다뤄야 하는 경우 `ScopedActor`가 ID 순으로 정렬해 일괄 잠금하여 데드락을 원천 차단합니다.

```cpp
// ScopedActor — 고정 순서 잠금 + 단계적 백오프로 데드락/라이브락 방지
bool ScopedActor::TryAcquire() {
    for (int32 retry = 0; retry < retryLimit; ++retry) {
        if (tryAcquireAll())  return true;   // ID 순 정렬 후 일괄 Acquire
        Release();                            // 실패 시 역순 전체 해제
        if      (retry < 10) _mm_pause();                // 스핀
        else if (retry < 50) std::this_thread::yield();   // 양보
        else              std::this_thread::sleep_for(1ms); // 백오프
    }
}
```

### Lock-free 자료구조 — ABA 방지 설계

| 자료구조 | 구현 방식 | ABA 방지 | 용도 |
|----------|-----------|----------|------|
| **LockFreeQueue** | Dummy-head 연결 리스트 + `atomic_ref` CAS + `Node16`(포인터 + 64-bit 버전 카운터) | 버전 카운터 (head/tail 별) | ActorMailbox · SendBuffer 큐의 기반 |
| **LockFreeStack** | Treiber 스택 + `Node16` wide-CAS | top 버전 카운터 | 세션 풀 · ConnectionPool 인덱스 관리 |

> **설계 원칙** · `alignas(hardware_destructive_interference_size)`로 head/tail/count를 별도 캐시 라인에 배치해 **false sharing을 제거**하고, 노드 할당은 TLS 캐시(`TlsObjectPool`)에서 처리하여 atomic 연산 오버헤드를 최소화했습니다.

### 계층형 TimingWheel — O(1) 타이머 스케줄링

하트비트·세션 타임아웃·지연 콜백을 위한 타이머를 **3레벨 계층 구조(256 / 256 / 128 슬롯)**로 구현했습니다. 표준 우선순위 큐 대비 삽입·만료가 **상수 시간**에 처리되며, 디스패치 루프에 통합되어 별도 타이머 스레드가 없습니다.

---

## 05 Memory & Multi-Server Systems

### 3계층 메모리 할당 전략

빈번한 패킷 송수신에서 할당 비용과 경합을 줄이기 위해, **스레드 로컬 캐시 → 글로벌 Lock-free 풀 → mimalloc(OS)**로 이어지는 3계층 전략을 설계했습니다.

```
L1 · TLS 캐시    TlsObjectPool<T>    스레드별 Chunk · atomic 불필요 (최단 경로)
      →
L2 · 글로벌 풀   ObjectPool<T>       Lock-free 프리리스트 · Chunk 재활용
      →
L3 · OS 할당     mimalloc            풀 고갈/초과 시 호출
```

| 항목 | 내용 |
|------|------|
| **안전성 · 무결성** | 각 노드에 체크섬 쿠키(`0xDEAD…BEEF`)를 심어 double-free·메모리 손상을 **런타임에 감지**. `MemoryPool`은 블록마다 소속 Chunk 역참조 포인터를 내장해 `Free()`를 O(1)로 처리 |
| **SendBufferAllocator** | 요청 크기가 4096B 이하면 256B, 초과 시 4096B 단위로 올림 정렬(최대 65536B)해 `SharedPtr<NetSendBuffer>` 발급. 패킷 크기 편차로 인한 할당 단편화를 억제 |

> **게임 서버 관점** · 초당 수많은 패킷이 오가는 MMORPG에서 할당 비용·단편화·메모리 손상은 곧 렉과 크래시로 직결됩니다. 이를 엔진 레벨에서 통제하는 구조입니다.

### 멀티 서버 · 수평 확장 설계

| 설계 | 내용 | 게임 적용 |
|------|------|-----------|
| **최소 연결 로드밸런싱** | 각 Gateway가 자신의 세션 수·상태를 Redis에 등록(`Gateway:*`). LoginServer의 `GatewaySelector`가 온라인 게이트웨이 중 현재 연결이 가장 적은 노드를 선택해 접속 분배 | 접속 폭주 시 서버 간 부하 분산·증설 |
| **Relay 패턴 · 투명 중계** | `Gateway ↔ World` 간 `RELAY_TO_CLIENT / RELAY_TO_WORLD`로 패킷을 투명 중계. 클라이언트는 단일 접속점만 알면 되고, 내부 서버는 역할에만 집중 | 접속 관문 분리·내부 토폴로지 은닉 |
| **범위 기반 채팅 라우팅** | 채팅을 LOCAL(AOI 범위)·WORLD(월드 전체)·REALM으로 구분해 패킷 릴레이. REALM 채팅은 RealmServer를 통해 여러 World로 S2S 전파 | 근거리·월드·렐름 범위별 채팅 전파 |
| **세션 신뢰성 · 자동 정리** | 양방향 하트비트 + Redis 세션 TTL + 중복 로그인 Kick(Redis Pub/Sub) + `SessionReaper` 자동 정리로 좀비 세션·자원 누수 차단 | 끊긴 접속 정리·중복 로그인·계정 보안 |

---

## 06 Server Authority — NavMesh · AOI · Codegen

### 서버 사이드 NavMesh — 서버 권위적 경로 탐색

이동 검증을 클라이언트가 아닌 서버에서 수행하기 위해, **UE5 Detour 라이브러리를 서버에 이식**하여 서버가 권위적으로 경로를 계산합니다. NavMesh 바이너리는 AWS S3에서 로드하며, DB(`world_navmesh_source`)로 버전·맵별 경로를 관리하고, 로딩 실패 시 **fail-fast**로 기동을 중단해 잘못된 상태로의 서비스 진입을 막습니다.

| 컴포넌트 | 내용 |
|----------|------|
| **FindPath** | `dtNavMeshQuery` 기반 start→end 경로 탐색, GameTick에서 경로 추종 |
| **AOI · GridManager** | 맵을 격자 셀로 분할(셀 크기 설정값). 셀 경계 이동 시 Enter/Leave 자동 브로드캐스트로 관심 영역만 동기화 |
| **S3 + IAM** | 최소 권한(`s3:GetObject`) IAM Role로 NavMesh 다운로드. 운영 보안을 고려한 권한 설계 |

> **AOI(Area of Interest)**는 "플레이어의 시야 안에 들어온 대상만 추려 전송"하는 구조로, 대규모 동접 환경에서 위치·상태 동기화 트래픽을 크게 줄이는 MMORPG의 핵심 기법입니다.

### PacketGenerator — 프로토콜 코드 자동 생성

`.proto`에 커스텀 옵션(`packet_id · sender · receiver`)으로 메타데이터를 선언하면, **역할(Role)별 수신 등록 코드·송신 헬퍼·통합 디스패처·PacketId enum**이 자동 생성됩니다. 현재 프로젝트 역할에 해당하는 메시지만 골라 코드를 만들고, 클라이언트가 실제로 쓰는 proto만 UE 모듈로 복사하여 서버·클라 프로토콜을 항상 동기화합니다.

```
// .desc(descriptor) → 역할별 C++ 코드 자동 생성 파이프라인
.proto  →  protoc  →  .pb.h / .desc  →  PacketGenerator.exe (.NET)
                                               ↓
          PacketId.h  ·  {Role}PacketHandler.h  ·  UE Protocol/*.pb.cpp (클라 복사)
```

---

## 07 Experience & Background

### 핵심 내러티브

잠시 현업을 떠나 진로를 깊이 고민한 시기를 거치며, 코딩이 한순간도 머릿속을 떠나지 않는다는 사실을 확인하고 분명한 확신을 갖고 돌아왔습니다. 그 확신은 "도구를 잘 쓰는 개발자"를 넘어 **서버 시스템의 근본을 만들 수 있는 개발자**가 되겠다는 방향으로 이어졌고, 그 결과물이 네트워크 엔진부터 분산 서버·서버 사이드 NavMesh·AOI까지 구현한 DH1_MMORPG입니다. 게임 서버의 안정성과 깊이를 동시에 추구하는 개발자가 되겠습니다.

### 게임 서버 개발자 적합성 요약

| 엔진 깊이 | 게임 시스템 | 라이브 실무 |
|------|------|------|
| IOCP·Lock-free·메모리 풀을 구현해 렉·크래시·동접 부하를 근본부터 다루는 역량 | NavMesh 서버 권위 이동·AOI·GameTick·채팅을 구현한 실제 MMORPG 서버 설계 경험 | 라이브 서비스와 신작 프로젝트 양쪽에서 콘텐츠·시스템 개발과 운영 이슈 대응·리팩토링을 수행한 실무 감각 |

> 주어진 일을 잘 해내는 것을 넘어, **더 나은 구조와 더 깊은 이해**를 향해 꾸준히 성장하는 엔지니어가 되겠습니다.

---

*정찬훈 · devhun001@gmail.com · 010-5158-2378*
