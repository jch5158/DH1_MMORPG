# World AOI · 클라이언트 가시 엔티티 구현 계획

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 스펙 [2026-04-01-world-aoi-client-visibility-design.md](../specs/2026-04-01-world-aoi-client-visibility-design.md)에 따라 **1단계(영향권 셀 집합 diff)** 로 서버의 가시 집합을 유지하고 `S2C_ENTITY_ENTER` / `S2C_ENTITY_LEAVE`로 클라와 동기화하며, 클라에서 원격 엔티티를 스폰·디스폰한다.

**Architecture:** `GridManager`는 그리드·셀·`GetObjectsInRange`만 담당한다. **가시성 결정과 패킷 생성**은 WorldServer의 **단일 모듈**(`AoiVisibilityService` 등)에 모은다. 플레이어(세션)마다 **서버 측 가시 엔티티 ID 집합**을 유지하고, 기준 셀 변화·오브젝트 스폰·이동·제거 시 집합 diff로 ENTER/LEAVE를 보낸다. 클라는 `entityId` → 월드 표현(액터) 맵을 두고 ENTER 시 생성·LEAVE 시 제거(멱등).

**Tech Stack:** C++17 WorldServer (CppNetEngine), Protobuf `Movement.proto`, UE5.7 클라이언트 `MovementPacketHandler` / `UClientNetSubsystem`, 기존 `S2S_RELAY_TO_CLIENT` 경로.

---

## 파일 구조 (책임 분리)

| 경로 | 책임 |
|------|------|
| **Create** `DH1_Server/WorldServer/AoiVisibilityService.h` | 가시 집합 상태, diff 계산 API, ENTER/LEAVE 버퍼 생성 선언 |
| **Create** `DH1_Server/WorldServer/AoiVisibilityService.cpp` | `GridManager`+`PlayerObject` 기반 구현; `sendRelayToClient` 호출은 콜백/인터페이스로 주입해 `GameTickProcessor`와 순환 참조 최소화 |
| **Modify** `DH1_Server/WorldServer/PlayerObject.h` / `.cpp` | (선택) `PlayerObject`에 가시 집합 보관 시 멤버 추가; 또는 `AoiVisibilityService` 내부 `unordered_map<actorId, set>` 만 사용해 Player는 건드리지 않음 — **후자 우선(YAGNI)** |
| **Modify** `DH1_Server/WorldServer/GameTickProcessor.cpp` | `processVisibilityChanges()`를 서비스 위임으로 교체; 기존 “새 셀 주변 전부 ENTER”만 보내는 로직 제거 |
| **Modify** `DH1_Server/WorldServer/GameTickProcessor.h` | `AoiVisibilityService` (unique_ptr 또는 ref) 멤버 |
| **Modify** `DH1_Server/WorldServer/WorldService.cpp` (또는 Tick 생성부) | `GameTickProcessor` 생성 시 서비스 조립 |
| **Modify** `DH1_Server/WorldServer/PacketHandler/GameSessionPacketHandler.cpp` | 스폰 직후 **초기 AOI 스냅샷**: 가시 집합 시드 + `S2C_ENTITY_ENTER_NOT` 일괄(본인 제외) |
| **Modify** `DH1_Server/WorldServer/GridManager.cpp` / `.h` | (필요 시) `AddObject` / `MoveObjectToCell` / `RemoveObject` 끝에서 `AoiVisibilityService::OnWorldObject*` 알림 — **오브젝트 생애주기 반영**에 필요 |
| **Modify** `DH1_Client/Source/DH1_Client/Network/Subsystem/ClientNetSubsystem.h` / `.cpp` | (또는 신규 서브시스템) 원격 `entityId` 레지스트리, 게임 스레드에서 스폰/디스패치 |
| **Modify** `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.cpp` | `HANDLE_S2C_ENTITY_ENTER_NOT` / `LEAVE`에서 서브시스템 호출 |
| **Reference** `Shared/Protocol/Proto/Movement.proto` | 필드 변경 없음(이미 ENTER/LEAVE 정의됨). 변경 시 PacketGenerator 재실행 |
| **Docs** | 본 플랜 완료 후 스펙의 “관찰 지표”에 맞춰 로그 키 한 줄 추가 여부만 결정 |

**테스트:** WorldServer용 C++ 단위 테스트 프로젝트는 현재 없음. **수동 검증**을 1차 완료 기준으로 한다. (로그인 서버만 .NET Tests 존재)

---

### Task 1: AOI 집합 수학·헬퍼 (서버, 테스트 가능한 순수 로직)

**Files:**
- Create: `DH1_Server/WorldServer/AoiVisibilityTypes.h` (optional — `using EntityIdSet = HashSet<uint64>` alias 등)
- Create: `DH1_Server/WorldServer/AoiVisibilityDiff.h` / `.cpp` — `GridManager`에 의존하는 **셀 범위 → 엔티티 ID 집합** 수집 함수

- [ ] **Step 1:** `CollectEntityIdsInAoi(GridManager&, centerCellX, centerCellY, excludeActorId)` 구현 — 내부에서 `GetCellsInRange` + 셀별 오브젝트 순회.
- [ ] **Step 2:** `ComputeEnterLeave(const EntityIdSet& previous, const EntityIdSet& current)` → `(toEnter, toLeave)` 벡터 또는 집합.
- [ ] **Step 3:** `DH1_Server/WorldServer/WorldServer.vcxproj` (또는 CMake)에 새 `.cpp` 추가.
- [ ] **Step 4:** 빌드 확인 — `MSBuild` 또는 솔루션 빌드.

```bash
# 예시 (환경에 맞게 조정)
dotnet build "E:\Projects\DH1_MMORPG\DH1_Server\DH1_Server.slnx" -c Debug --verbosity minimal
```

**Expected:** WorldServer 타깃 컴파일 성공.

- [ ] **Step 5:** 커밋 — `feat(world): AOI 엔티티 집합 수집·diff 헬퍼 추가`

---

### Task 2: AoiVisibilityService — 송신·상태 보관

**Files:**
- Create: `DH1_Server/WorldServer/AoiVisibilityService.h`
- Create: `DH1_Server/WorldServer/AoiVisibilityService.cpp`
- Modify: `DH1_Server/WorldServer/GameTickProcessor.cpp` — `sendRelayToClient` 시그니처를 람다로 전달하거나 `WorldService`에서 공유

- [ ] **Step 1:** 서비스에 `HashMap<uint64 /*viewerActorId*/, HashSet<uint64> /*visible*/>` 보관.
- [ ] **Step 2:** `void OnPlayerCellChanged(PlayerObject&, int32 oldCellX, int32 oldCellY, int32 newCellX, int32 newCellY)` — 이전 AOI 집합 vs 새 AOI 집합 diff → `S2C_ENTITY_LEAVE_NOT` (있으면) 후 `S2C_ENTITY_ENTER_NOT`. **기존 구현처럼 ENTER만 중복 폭주하지 않도록** 반드시 LEAVE 반영.
- [ ] **Step 3:** `void SeedInitialVisibility(PlayerObject&)` — 스폰 직후 한 번, 현재 셀 AOI 전체를 `visible`에 넣고 ENTER 패킷 전송(본인 제외).
- [ ] **Step 4:** `void OnWorldObjectAdded(GameObject&, cellX, cellY)` — AOI 안의 모든 **플레이어** viewer에 대해, 해당 엔티티가 아직 `visible`에 없으면 ENTER 추가 및 전송.
- [ ] **Step 5:** `OnWorldObjectRemoved(actorId)` — 모든 viewer의 `visible`에서 제거, LEAVE 전송.
- [ ] **Step 6:** `OnWorldObjectMoved(actorId, oldCellX, oldCellY, newCellX, newCellY)` — old AOI에만 있던 플레이어에 LEAVE, new에만 있던 플레이어에 ENTER (겹치는 플레이어는 무변경).

**Note:** 플레이어가 **처음** 스폰될 때 `oldCell`이 없으면 diff 대신 Seed만 호출.

- [ ] **Step 7:** 커밋 — `feat(world): AoiVisibilityService 가시 집합·ENTER/LEAVE 송신`

---

### Task 3: GameTickProcessor 연동

**Files:**
- Modify: `DH1_Server/WorldServer/GameTickProcessor.cpp` (대략 L70–L135)
- Modify: `DH1_Server/WorldServer/GameTickProcessor.h`
- Modify: `DH1_Server/WorldServer/WorldService.cpp` — 프로세서 생성자 인자

- [ ] **Step 1:** 플레이어에 대해 **이전 틱의 셀 좌표**를 저장할 필드 추가 — `PlayerObject`에 `mLastVisibilityCellX/Y` (또는 서비스 내부 map). 초기값은 “미설정” 플래그로 Seed 트리거.
- [ ] **Step 2:** `processVisibilityChanges`에서 모든 오브젝트 순회 시 **플레이어 셀 변경 감지** 후 `MoveObjectToCell` **이전**에 old 셀 좌표를 캡처하고, 이동 **이후** `OnPlayerCellChanged` 호출.
- [ ] **Step 3:** 비플레이어의 셀 변경 시 `OnWorldObjectMoved` 호출 (이전/새 셀 좌표 필요 — `MoveObjectToCell` 내부 또는 호출 전후로 전달).
- [ ] **Step 4:** 빌드 및 로컬 2클라 이동 시 로그로 ENTER/LEAVE 순서 확인.

- [ ] **Step 5:** 커밋 — `feat(world): 틱 가시성을 AoiVisibilityService에 위임`

---

### Task 4: GridManager 생애주기 훅

**Files:**
- Modify: `DH1_Server/WorldServer/GridManager.h` / `.cpp`
- Modify: `DH1_Server/WorldServer/WorldService.h` / `.cpp` — `AoiVisibilityService` 싱글톤 또는 `WorldService`가 보유 후 Grid에 포인터 전달 (YAGNI: 콜백 std::function 1개도 가능)

- [ ] **Step 1:** `AddObject` 성공 후 `visibility->OnWorldObjectAdded(...)`.
- [ ] **Step 2:** `RemoveObject`에서 셀에서 제거 직전에 위치/셀 정보로 `OnWorldObjectRemoved`.
- [ ] **Step 3:** `MoveObjectToCell`에서 old/new 셀로 `OnWorldObjectMoved`.

**주의:** `PlayerObject`가 스폰될 때 `AddObject`와 `GameSessionPacketHandler`의 Seed가 **이중 ENTER** 되지 않게 순서 정의 — 보통 **Seed를 AddObject 이후 한 번만** 호출하고, `OnWorldObjectAdded`는 **플레이어가 아닌 타입**에만 브로드캐스트하거나, 플레이어는 Seed에서만 처리.

- [ ] **Step 4:** 커밋 — `feat(world): 그리드 오브젝트 추가·이동·제거 시 AOI 브로드캐스트`

---

### Task 5: GameSessionPacketHandler 초기 스폰

**Files:**
- Modify: `DH1_Server/WorldServer/PacketHandler/GameSessionPacketHandler.cpp` (스폰 성공 분기, `AddObject` 근처)

- [ ] **Step 1:** `pGridManager->AddObject` 이후 `AoiVisibilityService::SeedInitialVisibility(*pPlayerObject)` 호출.
- [ ] **Step 2:** 기존에 별도로내던 중복 ENTER와 겹치지 않는지 확인 — `processVisibilityChanges`와 **한 경로만** 초기 주변을 책우도록 정리.

- [ ] **Step 3:** 커밋 — `feat(world): 스폰 직후 초기 AOI ENTER 시드`

---

### Task 6: 클라이언트 ENTER/LEAVE 처리

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/Subsystem/ClientNetSubsystem.h` / `.cpp`
- Modify: `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.cpp`

- [ ] **Step 1:** `TMap<uint64, TObjectPtr<AActor>>` 또는 전용 컴포넌트 보유 — **원격** 엔티티만 (로컬 플레이어 제외 규칙 명시).
- [ ] **Step 2:** `HANDLE_S2C_ENTITY_ENTER_NOT`: 게임 스레드에서 블루프린트/ C++ 스폰 팩토리 호출 — 기존 `S2C_SPAWN_POSITION_RES`와 동일한 캐릭터 클래스 재사용 가능 여부 검토 (YAGNI: 간단한 `ADH1_RemoteCharacter` 또는 기존 `ADH1_ClientCharacter` 복제 모드).
- [ ] **Step 3:** `HANDLE_S2C_ENTITY_LEAVE_NOT`: 맵에 있으면 `DestroyActor`, 없으면 무시 (**멱등**).
- [ ] **Step 4:** 월드 종료·로그아웃 시 맵 클리어.

- [ ] **Step 5:** PIE에서 2클라(또는 서버 로그만)로 ENTER/LEAVE 수신 확인.

- [ ] **Step 6:** 커밋 — `feat(client): ENTITY_ENTER/LEAVE로 원격 엔티티 스폰·디스폰`

---

### Task 7: 스냅샷과의 관계 정리

**Files:**
- Modify: `DH1_Server/WorldServer/GameTickProcessor.cpp` — `broadcastSnapshots`

- [ ] **Step 1:** 스냅샷은 **이미 ENTER로 알려진 엔티티**에만 보내는지 검토 — 클라에 없는 ID를 스냅샷만으로 갱신하면 안 됨. 서버에서 `visible` 집합과 교집합만 스냅샷에 포함하도록 필터(권장).
- [ ] **Step 2:** 커밋 — `fix(world): 스냅샷을 AOI 가시 집합과 일치`

---

### Task 8: 부하·관측 (5k 대비, 선택)

**Files:**
- Modify: `AoiVisibilityService.cpp` — 카운터 또는 `NET_ENGINE_LOG_DEBUG` 빈도 제한

- [ ] **Step 1:** 틱당 `enterCount`/`leaveCount` 누적 후 N초마다 한 줄 로그(옵션).
- [ ] **Step 2:** 커밋 — `chore(world): AOI diff 관측 로그(옵션)`

---

## 수동 검증 체크리스트 (완료 정의)

1. 클라 A·B 동시 접속, 같은 AOI 안에서 서로 **ENTER** 후 상대 메시가 보이거나(또는 로그) 스폰됨.
2. A가 멀리 이동해 B의 AOI 밖으로 나가면 B에게 **LEAVE**, A에게 B에 대한 **LEAVE**(대칭).
3. 서버에서 NPC(또는 테스트 오브젝트) **스폰** 시 AOI 내 플레이어만 ENTER.
4. **제거** 시 LEAVE.
5. 빠른 셀 경계 왕복 시 크래시 없음, 클라 멱등(중복 LEAVE/ENTER).

---

## Plan review loop

1. (선택) `plan-document-reviewer`에 본 플랜 경로 + 스펙 경로를 넘겨 검토.
2. 지적 사항 있으면 본 문서 수정 후 재검토(최대 3회).

---

## Execution handoff

**플랜 저장 위치:** `docs/superpowers/plans/2026-04-01-world-aoi-visibility-implementation.md`

**실행 방식 선택:**

1. **Subagent-Driven (권장)** — 태스크마다 새 서브에이전트, 태스크 간 리뷰. **필수:** superpowers:subagent-driven-development  
2. **Inline Execution** — 이 세션에서 executing-plans로 일괄 진행, 체크포인트마다 리뷰. **필수:** superpowers:executing-plans  

원하는 번호를 알려 주면 그에 맞춰 진행하면 됩니다.
