# 채팅 콤보 대비 + AOI 엔터티 갱신 — 구현 플랜

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** (1) 채팅 채널 콤보 드롭다운을 읽을 수 있는 대비로 만든다. (2) AOI 관련 서버→클라 흐름과 클라 `GetWorld`/스폰 경로를 런타임 증거로 확인한 뒤, 입증된 결함만 수정한다.

**Architecture:** 클라 `ClientNetSubsystem`에서 콤보 행 위젯에 명시적 배경·글자색 적용. AOI는 `MovementPacketHandler` → `ApplyNetworkEntitiesEntered`와 월드 `GameTickProcessor`·릴레이 체인을 로그로 교차 검증.

**Tech Stack:** UE 5.7 UMG (`UBorder`, `UTextBlock`, `UComboBoxString`), C++ 클라/월드/게이트웨이 서버, 기존 Protocol 이동 패킷.

**Spec:** `docs/superpowers/specs/2026-04-01-chat-combo-contrast-aoi-update-design.md`

---

## 파일 맵

| 영역 | 파일 |
|------|------|
| 콤보 UI | `DH1_Client/Source/DH1_Client/Network/Subsystem/ClientNetSubsystem.cpp` (`ApplyDarkChatComboDropdownStyle`, `HandleChatComboGenerateItem`, `RegisterChatUi`) |
| 클라 AOI | `DH1_Client/.../MovementPacketHandler.cpp`, `ClientNetSubsystem.cpp` (`ApplyNetworkEntitiesEntered`, `ForEachPlayClientNetSubsystem`) |
| 서버 AOI | `DH1_Server/WorldServer/GameTickProcessor.cpp`, `DH1_Server/WorldServer/PacketHandler/GameSessionPacketHandler.cpp` |
| 게이트웨이 | `DH1_Server/GatewayServer/PacketHandler/GameSessionPacketHandler.cpp` (`HANDLE_S2S_RELAY_TO_CLIENT_NOT`) |

---

### Task 1: 채팅 콤보 가독성 (코드 수정)

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/Subsystem/ClientNetSubsystem.cpp`

- [ ] **Step 1:** `HandleChatComboGenerateItem`에서 `UBorder`에 **어두운 배경** 설정 (`SetBrushColor` 또는 `Background`용 `FSlateBrush` 틴트 — 엔진 API에 맞게 선택).
- [ ] **Step 2:** `UTextBlock` 색을 배경과 대비되게 유지/조정 (기존 0.92 밝은색은 **어두운 배경**과 함께면 유지 가능).
- [ ] **Step 3:** PIE에서 **닫힌 콤보 버튼**과 **펼친 드롭다운 행(및 필요 시 패널 주변)** 을 각각 확인 — 행만 정상이고 바깥이 하얗면 `WidgetStyle` 메뉴/패널 쪽 추가 조정 검토.
- [ ] **Step 4:** 빌드: `Build.bat` 또는 `UnrealBuildTool`로 `DH1_ClientEditor` Win64 Development 성공 확인.
- [ ] **Step 5:** 커밋 예: `fix(client): chat channel combo dropdown contrast for generated rows`

---

### Task 2: AOI — 런타임 증거 수집 (기존 계측 활용 또는 최소 추가)

**가설 ID (플랜·로그와 1:1 매핑):**

| ID | 내용 |
|----|------|
| H1 | 게임 스레드 배치는 도착하나 `batch==0` 등 클라 큐 문제 |
| H2 | `ApplyNetworkEntitiesEntered`에서 `GetWorld()==nullptr` |
| H3 | 월드→게이트웨이 `sendRelayToClient` / 세션 ID 불일치 |
| H4 | 클라 패킷 서비스 타입·opcode → 핸들러 미연결 또는 조기 return |
| H5 | 서버가 ENTER 자체를 보내지 않음(스폰/범위/셀 로직) |

**Files:**
- Read/Use: 워크스페이스 루트 `debug-<session>.log` (NDJSON) 또는 동등 경로
- Optional modify: `ClientNetSubsystem.cpp`, `MovementPacketHandler.cpp`, `GameTickProcessor.cpp` — **가설당** 소량(보통 **가설당 1~2줄**, 전체 **10줄 이하** 권장).

- [ ] **Step 1:** 재현 전 **세션 로그 파일만** 비운다 (에이전트: `delete_file`; 수동: 해당 `debug-*.log` 삭제).
- [ ] **Step 2:** 코드로 `ENTER`/`SNAPSHOT`/`LEAVE`가 모두 `ApplyNetworkEntitiesEntered` / `ApplyNetworkEntitiesLeft`로 수렴하는지 **호출 경로 한 번 훑기**.
- [ ] **Step 3:** 2클라 + 서버, 이동·스폰 시나리오 실행.
- [ ] **Step 4:** 로그에서 **H2** `hasWorld:0` 여부 확인 → null이면 Task 3a 우선; `hasWorld:1`이고 `batch>0`인데 화면에 없으면 **H4** 클라 디스패치·맵 로직으로 분기.
- [ ] **Step 5:** 월드 서버 로그에 ENTER 릴레이 전송 여부 확인(필요 시 한 줄 `NET_ENGINE_LOG` 추가 후 재빌드). `gatewaySessionId`가 게이트웨이 `GetSessionBySessionId`와 맞는지 Task 3b와 연계.
- [ ] **Step 6:** 가설별 CONFIRMED/REJECTED를 스펙/이슈에 한 단락으로 기록.

---

### Task 3a: 클라 — World 컨텍스트 (H2 CONFIRMED 시에만)

**Files:**
- Modify: `ClientNetSubsystem.cpp` (및 필요 시 `MovementPacketHandler.cpp`)

- [ ] **Step 1:** `ApplyNetworkEntitiesEntered`에서 `GetWorld()`가 null일 때 **단일** 대체 경로(예: `ForEachPlayClientNetSubsystem`에서 이미 알고 있는 `Context.World()`를 인자로 넘기기, 또는 서브시스템 초기화 시 캐시) 구현.
- [ ] **Step 2:** 빌드 + 2클라 재현, 로그에 `hasWorld:1` 및 스폰 확인.
- [ ] **Step 3:** 커밋 예: `fix(client): resolve world for network entity spawn when subsystem GetWorld is null`

---

### Task 3b: 서버/릴레이 (H2·H4 REJECTED·H3/H5 CONFIRMED 시)

**Files:**
- Modify: `GameTickProcessor.cpp`, `GameSessionPacketHandler.cpp` (월드/게이트웨이), 스폰 경로 `GameSessionPacketHandler.cpp` (월드)

- [ ] **Step 1:** `sendRelayToClient`가 월드가 가진 **gatewaySessionId**와 게이트웨이 `HANDLE_S2S_RELAY_TO_CLIENT_NOT`가 찾는 **세션 키**가 동일 체계인지 확인. 빈 페이로드 없음.
- [ ] **Step 2:** 스폰 직후 `NotifyNearbyPlayersAboutEntity` / `NotifySpawnedPlayerVisibleEntities` 누락 경로가 있으면 **한 곳**에만 보강.
- [ ] **Step 3:** WorldServer 바이너리 재빌드 + 통합 테스트.
- [ ] **Step 4:** 커밋 예: `fix(world): ensure AOI enter relay to gateway clients on spawn/cell change`

---

### Task 3c: 클라 디스패치·앱 로직 (H2·H3 REJECTED, 로그에 batch>0·스폰 경로 정상인데 화면 없음)

**Files:**
- Modify: `MovementPacketHandler.cpp`, `PacketServiceTypeHandler` / 클라 이동 패킷 등록부, `ClientNetSubsystem.cpp`

- [ ] **Step 1:** `S2C_ENTITY_ENTER_NOT` / `SNAPSHOT` / `LEAVE` 헤더의 서비스 타입·packet id가 디스패치 맵과 일치하는지 확인.
- [ ] **Step 2:** `NetworkEntityActors` 갱신·필터(로컬 플레이어 ID 제외 등)가 전부 제거·스킵하지 않는지 확인.
- [ ] **Step 3:** 빌드 + 재현 후 커밋.

---

### Task 4: 계측 정리 (사용자 승인 후)

- [ ] **Step 1:** post-fix 로그로 성공 입증 후 `#region agent log` 블록 제거.
- [ ] **Step 2:** 커밋 예: `chore: remove debug session logging for chat/aoi`

---

## Plan review loop

1. `code-reviewer`(또는 `plan-document-reviewer`)에 **본 플랜 경로 + 스펙 경로**만 전달해 검토.
2. 지적 반영 후 재검토(최대 3회).

## Execution handoff

플랜 저장: `docs/superpowers/plans/2026-04-01-chat-combo-contrast-aoi-update-plan.md`

**실행 옵션:** (1) Subagent-Driven — 태스크마다 새 서브에이전트 (2) Inline — 본 세션에서 순차 실행. 원하는 방식을 지정하면 그에 맞춰 진행.
