# Chat bar + AOI verification — implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 채팅 하단 행(채널 콤보·입력·전송)의 폰트·패딩·세로 정렬을 통일하고, AOI 관련 서버/게이트웨이/클라 경로를 로그로 검증한다.

**Architecture:** `ClientNetSubsystem::RegisterChatUi`에서 `WBP_Chat` 네임드 위젯에 스타일/슬롯을 적용. AOI는 기존 WorldServer 릴레이·클라 `MovementPacketHandler`·NDJSON 로그로 런타임 증거를 수집한다.

**Tech stack:** Unreal Engine 5.7 (UMG), DH1 C++ client, C++ WorldServer/GatewayServer.

---

### Task 1: Chat bar runtime unification

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/Subsystem/ClientNetSubsystem.cpp`

- [x] **Step 1:** `ApplyLightChatComboDropdownStyle` / `HandleChatComboGenerateItem` / `RegisterChatUi`에 동일 `FMargin(10,5,10,5)` 및 `Combo->GetFont()` 기반 입력란 `TextStyle` 적용.
- [x] **Step 2:** `UHorizontalBoxSlot`에 `VAlign_Center` 적용(콤보·입력·버튼).
- [x] **Step 3:** `Btn_Send` 직계 자식 `UTextBlock`에 동일 폰트 설정(있을 때만).
- [x] **Step 4:** `Build.bat DH1_Client Win64 Development` — 성공 확인.
- [ ] **Step 5:** 에디터/게임에서 콤보 선택 전후로 하단 행 높이·폰트가 일치하는지 수동 확인.

### Task 2: AOI runtime evidence

**Files:**
- Reference: `DH1_Server/WorldServer/GameTickProcessor.cpp`, `DH1_Server/GatewayServer/PacketHandler/GameSessionPacketHandler.cpp`
- Reference: `DH1_Client/.../MovementPacketHandler.cpp`, `debug-ab4bf2.log`

- [ ] **Step 1:** WorldServer **재빌드·재시작** 후 두 클라이언트로 접속.
- [ ] **Step 2:** 프로젝트 루트 `debug-ab4bf2.log`에서 `SNAPSHOT_gt`의 `batch`/`firstEntityId`, `ApplyNetworkEntitiesEntered`의 `count` 및 **`firstPos`(좌표 샘플)** 확인.
- [ ] **Step 3:** Gateway 로그에서 `Client session not found for relay` 유무 확인.
- [ ] **Step 4:** 증거에 따라 다음 중 하나: (a) 서버 미배포, (b) 세션 ID 불일치, (c) 좌표 스케일, (d) 클라 `ForEach` 미호출 — 원인별 후속 태스크 분기.

### Task 3: Commit

- [ ] **Step 1:** 클라 변경 커밋 메시지 예: `fix(client): unify WBP_Chat row font padding and vertical alignment`

---

## Plan review

코드 리뷰어 서브에이전트에게 본 플랜과 `ClientNetSubsystem.cpp`의 `RegisterChatUi` 변경 구간 검토를 요청한다.
