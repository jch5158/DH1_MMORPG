# 인게임 채팅 (일반·월드·렐름 확성기) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 스펙 `docs/superpowers/specs/2026-03-31-in-game-chat-design.md` 대로 `C2S_CHAT_REQ` / `S2C_CHAT_NOT` 및 렐름 팬아웃을 구현하고, UE 클라이언트에서 수신·송신·최소 UI를 연결한다.

**Architecture:** Gateway는 Movement와 동일하게 `S2S_RELAY_TO_WORLD_NOT`로 Chat inner payload를 World에 넘긴다. World의 `HANDLE_S2S_RELAY_TO_WORLD_NOT`에 `SERVICE_TYPE_CHAT` 분기를 추가한다. **브로드캐스트·검증·레이트 리밋은 얇은 핸들러가 아니라 `WorldServer/Chat/` 모듈(`WorldChatBroadcastService` 등)에 모아** 유지보수성을 확보한다. 렐름 확성기는 World→Realm `S2S_REALM_CHAT_SUBMIT_NOT`, Realm이 `RealmSessionRegistry`를 월드별로 그룹화해 `S2S_REALM_CHAT_DELIVER_NOT`로 각 World에 전달, World가 `GameTickProcessor::sendRelayToClient`와 동일 패턴으로 `S2S_RELAY_TO_CLIENT_NOT`를 쓴다.

**Tech Stack:** Protobuf (`Shared/Protocol`), 기존 `PacketGenerator`, C++ World/Gateway/Realm 서버, UE 5.x `UClientNetSubsystem` + 자동 생성 `*PacketHandler`.

**유지보수·설계 원칙 (필수):**
- 채팅 **비즈니스 로직**은 `PacketHandler/*Chat*`의 `HANDLE_*` 한두 줄 delegate로 끝나게 하고, 실제 구현은 `WorldServer/Chat/` 아래 집중한다.
- 문자열 검증(UTF-8 길이, 최대 바이트, 금지 제어문자)은 **한 함수/네임스페이스**로 공유한다. 스폰 시트 `displayName` 상한과 **동일 바이트 상한을 쓸지** Task 3 시작 전에 한 줄로 결정해 상수 중복을 피한다.
- 레이트 리밋은 계정 단위, 채널별 상수 테이블(LOCAL 완화 / REALM 엄격)로 한 곳에서 조정 가능하게 한다.
- Realm의 `worldServerId → WorldServerSession` 해석은 **`RealmService` 또는 전용 `RealmWorldSessionLookup`** 한 곳에만 두고, `PacketHandler`는 호출만 한다.
- **릴레이 조립**은 `GameTickProcessor::sendRelayToClient`와 **동일한 Gateway 세션 선택 규칙**을 쓰도록, 가능하면 **공용 헬퍼 한 곳**으로 추출해 틱·채팅이 갈라지지 않게 한다.

**보안·신뢰 경계 (필수):**
- `S2S_REALM_CHAT_SUBMIT_NOT`의 **송신자 `account_id`, `display_name`, 타임스탬프**는 **오직 권위 있는 서버 상태**(`RelayContext` + `PlayerObject` / DB 시트)에서만 채운다. 클라이언트가 보낸 `C2S_CHAT_REQ`에 송신자 식별 필드가 있더라도 **Realm 검증에 쓰지 않는다**.
- `S2S_REALM_CHAT_DELIVER_NOT`의 `s2c_payload`는 **World가 SUBMIT 직전에 직렬화한 `S2C_CHAT_NOT` 바이트**를 Realm이 그대로 전달하는 모델이면 Realm은 payload 내용을 재해석하지 않는다(신뢰: World→Realm 링크). World는 **자신에게만 연결된 Realm**에서 온 DELIVER만 처리한다(기존 세션 모델 유지).

---

## 파일 맵 (생성·수정)

| 역할 | 경로 |
|------|------|
| 스펙 | `docs/superpowers/specs/2026-03-31-in-game-chat-design.md` |
| Enum | `Shared/Protocol/Proto/Enum.proto` — `SERVICE_TYPE_CHAT`, `eChatChannel` |
| 신규 Proto | `Shared/Protocol/Proto/Chat.proto` — `C2S_CHAT_REQ`, `S2C_CHAT_NOT`, `S2S_REALM_CHAT_SUBMIT_NOT`, `S2S_REALM_CHAT_DELIVER_NOT` (각 메시지 `sender`/`receiver` 옵션은 GameSession·Movement와 동일 규칙 준수) |
| 생성물 | `Shared/Protocol/Chat.pb.h`, `Chat.pb.cc`, `packet_id` 갱신, 각종 `*PacketHandler*` 자동 생성 |
| World | `DH1_Server/WorldServer/WorldServer.vcxproj` — `Chat.pb.cc`, 신규 `Chat/*.cpp` |
| Gateway | `DH1_Server/GatewayServer/GatewayServer.vcxproj`, `PacketHandler/ChatPacketHandler.*`, `PacketServiceTypeHandler`, `MovementPacketHandler` 패턴 복제 |
| Realm | `DH1_Server/RealmServer/RealmServer.vcxproj`, `PacketHandler/GameSessionPacketHandler` 또는 `RealmChatPacketHandler`, `PacketServiceTypeHandler` |
| Client | `DH1_Client/.../Network/Protocol` (복사본), `PacketHandler/ChatPacketHandler`, `ClientNetSubsystem`, (선택) UMG 채팅 위젯 |

---

### Task 1: 프로토콜·빌드 파이프라인

**Files:**
- Modify: `Shared/Protocol/Proto/Enum.proto`
- Create: `Shared/Protocol/Proto/Chat.proto`
- Modify: `DH1_Server/WorldServer/WorldServer.vcxproj`, `GatewayServer.vcxproj`, `RealmServer.vcxproj`
- Run: `Shared/Tools/PacketGenerator` (또는 `Shared/BuildScripts/BuildAll.bat` 중 proto 단계)

- [ ] **Step 1:** `Enum.proto`에 `SERVICE_TYPE_CHAT = 8`, `eChatChannel` 3값 추가. **기존 enum 값과 충돌 없는지 `grep`/리뷰로 확인.**
- [ ] **Step 2:** `Chat.proto` 작성. `C2S_CHAT_REQ`(CLIENT→GATEWAY), `S2C_CHAT_NOT`(GATEWAY→CLIENT inner), `S2S_REALM_CHAT_SUBMIT_NOT`(WORLD→REALM), `S2S_REALM_CHAT_DELIVER_NOT`(REALM→WORLD), `RelayTarget` 서브메시지 정의.
- [ ] **Step 3:** PacketGenerator 실행 후 `Chat.pb.*`, `PacketId.h`, 핸들러 스텁 생성 확인.
- [ ] **Step 4:** World/Gateway/Realm `vcxproj`에 `Chat.pb.cc` 및 필요 시 `Proto/Chat.proto` None 항목 추가(기존 Movement 패턴 복제). **공용 정적 라이브러리에 `.pb.cc`를 모은다면 Chat은 한 곳만 링크**해 이중 링크/ODR 문제를 피한다.
- [ ] **Step 5:** `dotnet build` 또는 해당 서버 프로젝트 MSBuild로 컴파일 확인.
- [ ] **Step 6:** Commit — `feat(proto): add chat messages and SERVICE_TYPE_CHAT`

```bash
git add Shared/Protocol DH1_Server/*/*.vcxproj
git commit -m "feat(proto): add chat channel enums and Chat.proto"
```

---

### Task 2: Gateway — 클라 수신 검증 및 World 릴레이

**Files:**
- Create: `DH1_Server/GatewayServer/PacketHandler/ChatPacketHandler.h` / `.cpp` (생성기가 이미 만들었다면 수정만)
- Modify: `DH1_Server/GatewayServer/PacketHandler/PacketServiceTypeHandler.*` — `ChatPacketHandler::Init` 등록
- Modify: `DH1_Server/GatewayServer/PacketHandler/GameSessionPacketHandler.cpp` — 필요 시 없음; Chat은 별도 서비스 타입

- [ ] **Step 1:** `HANDLE_C2S_CHAT_REQ`: `ClientSession`이 `IsLoggedIn()` && `IsInWorld()` 일 때만 처리 (Movement `Validate`와 동일).
- [ ] **Step 2:** Inner buffer 구성: `PacketHeader` + serialized `C2S_CHAT_REQ`, `header->id` = `SERVICE_TYPE_CHAT | packet_id`.
- [ ] **Step 3:** `S2S_RELAY_TO_WORLD_NOT`로 World 세션에 전송 (`MovementPacketHandler` 66–70행 패턴 복사).
- [ ] **Step 4:** **검증** — World→Gateway `S2S_RELAY_TO_CLIENT_NOT` 경로가 **서비스 타입/패킷 ID와 무관하게 raw payload를 클라에 그대로 전달**하는지 확인한다. 그렇다면 `S2C_CHAT_NOT`용 Gateway 추가 분기는 불필요; 아니면 `GameSessionPacketHandler` 등에서 필요한 변경을 이 태스크에 명시한다.
- [ ] **Step 5:** 빌드 확인.
- [ ] **Step 6:** Commit — `feat(gateway): relay C2S_CHAT_REQ to world`

---

### Task 3: World — 릴레이 디스패치 + Chat 모듈 뼈대

**Files:**
- Modify: `DH1_Server/WorldServer/PacketHandler/GameSessionPacketHandler.cpp` — `HANDLE_S2S_RELAY_TO_WORLD_NOT`의 `switch (serviceType)`에 `SERVICE_TYPE_CHAT` 분기
- Create: `DH1_Server/WorldServer/Chat/WorldChatBroadcastService.h`, `WorldChatBroadcastService.cpp`
- Create: `DH1_Server/WorldServer/Chat/ChatMessageValidation.h`, `.cpp` (또는 `.inl` + 단일 cpp)
- Create: `DH1_Server/WorldServer/Chat/ChatRateLimiter.h`, `.cpp`
- Modify: `DH1_Server/WorldServer/WorldServer.vcxproj` — 새 cpp 추가
- Create/Modify: `DH1_Server/WorldServer/PacketHandler/ChatPacketHandler.*` — `HANDLE_C2S_CHAT_REQ`에서 `WorldChatBroadcastService::HandleClientChat(accountId, packet)` 호출

- [ ] **Step 1:** `RelayContext::GetAccountId()`와 `C2S_CHAT_REQ` 파싱은 기존 `ChatPacketHandler` 템플릿 경로 사용.
- [ ] **Step 2:** `WorldChatBroadcastService::HandleClientChat`:
  - `GridManager`에서 발신 `PlayerObject` 조회 실패 시 로그 후 return
  - `ChatMessageValidation::NormalizeAndValidate(message)` 실패 시 return (클라에 에러 패킷은 스펙 비목표면 생략 가능)
  - `ChatRateLimiter::Allow(accountId, channel)` 실패 시 return
  - `channel` 분기: LOCAL / WORLD / REALM
- [ ] **Step 3:** LOCAL: `ComputeCellX/Y` → `GetObjectsInRange` → 플레이어만 순회하며 `S2C_CHAT_NOT` 빌드 후 릴레이 헬퍼 호출.
- [ ] **Step 4:** WORLD: `GridManager::GetAllObjects()`에서 Player만 필터해 동일 릴레이. **스폰 직후 그리드에 없는 세션만 존재하는지 운영 경험상 가능하면** `GameSessionManager`와 소스 오브 트루스를 맞추거나, “WORLD = 그리드에 올라온 플레이어만”을 제품 정의로 문서화한다.
- [ ] **Step 5:** REALM: `S2C_CHAT_NOT` 직렬화 바이트 생성 후 `S2S_REALM_CHAT_SUBMIT_NOT`를 `RealmSession`으로 전송 (기존 `S2S_GAME_SESSION_SYNC_*` 전송 패턴 참고). **SUBMIT 메시지의 송신자 메타데이터는 전부 권위 있는 `PlayerObject`/세션에서만 설정.**
- [ ] **Step 6:** `S2C_CHAT_NOT` → `S2S_RELAY_TO_CLIENT_NOT` 조립은 **한 함수** `SendChatToClient(...)`로 묶어 **가능하면 `GameTickProcessor::sendRelayToClient`와 공유**한다(필요 시 짧은 리팩터로 중복 제거).
- [ ] **Step 7:** 빌드 확인.
- [ ] **Step 8:** Commit — `feat(world): chat broadcast service and relay dispatch`

---

### Task 4: World — Realm에서 오는 확성기 배달

**Files:**
- Modify: `DH1_Server/WorldServer/PacketHandler/GameSessionPacketHandler.h` / `.cpp` — `HANDLE_S2S_REALM_CHAT_DELIVER_NOT` 등록(생성기가 GameSession에 넣었는지 확인; **Chat.proto의 S2S가 GameSession이 아니면 ChatPacketHandler 쪽**에 핸들러 추가)
- Modify: `WorldChatBroadcastService` — `DeliverRealmChat(const Protocol::S2S_REALM_CHAT_DELIVER_NOT&)` 구현

- [ ] **Step 1:** PacketGenerator 출력 기준으로 **DELIVER 핸들러가 어느 `*PacketHandler*`에 붙는지 확인**하고, World의 해당 `HANDLE_*`에서 `WorldChatBroadcastService::DeliverRealmChat` 호출.
- [ ] **Step 2:** `targets` 반복: 각 `gateway_session_id` / `gateway_server_id`에 대해 `s2c_payload` bytes를 **그대로** `S2S_RELAY_TO_CLIENT_NOT`의 payload로 보내거나, 신뢰 경로만 재직렬화(권장: **직렬화된 blob 그대로 릴레이**해 CPU 절약).
- [ ] **Step 3:** 빌드 확인.
- [ ] **Step 4:** Commit — `feat(world): handle realm chat deliver from realm server`

---

### Task 5: Realm — 팬아웃 및 World 세션 라우팅

**Files:**
- Create: `DH1_Server/RealmServer/RealmWorldSessionLookup.h` / `.cpp` (또는 `RealmService` 내 private 메서드)
- Modify: `DH1_Server/RealmServer/PacketHandler/*` — `HANDLE_S2S_REALM_CHAT_SUBMIT_NOT`
- Modify: `RealmServer.vcxproj`

- [ ] **Step 1:** `RealmService::GetServerServiceRef()->GetActiveSessions()` 순회, `dynamic_pointer_cast<WorldServerSession>`, `GetWorldServerId() != 0` 인 세션만으로 `HashMap<int32, PacketSessionRef>` 구축. **동일 `worldServerId`로 둘 이상의 활성 세션이 있으면** 로그 + 결정적 한 세션 선택(예: 최신 하트비트) 또는 거부 정책을 코드와 주석으로 고정한다.
- [ ] **Step 2:** `HANDLE_S2S_REALM_CHAT_SUBMIT_NOT`: 송신자 `accountId`가 레지스트리에 있고 `worldServerId`가 일치하는지 검증(스푸핑 방지).
- [ ] **Step 3:** 레지스트리 전 순회로 `worldServerId` → `Vector<RelayTarget>` 그룹화(발신자 포함 전원에게 확성기 전달).
- [ ] **Step 4:** 월드별로 `S2S_REALM_CHAT_DELIVER_NOT` 전송; 해당 World 연결이 없으면 로그.
- [ ] **Step 5:** 빌드 확인.
- [ ] **Step 6:** Commit — `feat(realm): fan out realm chat to world servers`

---

### Task 6: Realm PacketServiceTypeHandler 등록

**Files:**
- Modify: `DH1_Server/RealmServer/PacketHandler/PacketServiceTypeHandler.*`

- [ ] **Step 1:** 생성된 `packet_id` / 서비스 타입에 맞게 `S2S_REALM_CHAT_SUBMIT_NOT` 디스패치 연결.
- [ ] **Step 2:** 빌드 확인.

---

### Task 7: UE 클라이언트 — 송수신

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/Subsystem/ClientNetSubsystem.h` / `.cpp`
- Modify/Create: `DH1_Client/.../Network/PacketHandler/ChatPacketHandler.*`
- Run: PacketGenerator → Client 프로토콜 복사

- [ ] **Step 1:** PacketGenerator 후 `DH1_Client` 모듈의 Protocol 폴더에 `Chat.pb.cpp` 등 반영 확인 (`DH1_Client.Build.cs` 경로 유지).
- [ ] **Step 2:** `HANDLE_S2C_CHAT_NOT`: 게임 스레드에서 `UClientNetSubsystem::NotifyChatMessage(...)` 같은 멀티캐스트 델리게이트 호출(채널, 보낸이, 본문, 타임스탬프).
- [ ] **Step 3:** `UClientNetSubsystem::RequestSendChat(channel, message)` — 내부에서 `C2S_CHAT_REQ` 직렬화 + `SendPacket` (Movement 송신 패턴 참고).
- [ ] **Step 4:** PIE에서 로그로 수신 확인.
- [ ] **Step 5:** Commit — `feat(client): chat packet send and notify delegate`

---

### Task 8: UE 최소 UI

**Files:**
- Create: `DH1_Client/.../UI/UMG/WBP_ChatLog` 또는 Slate — 프로젝트 관례에 맞게 1개
- 위젯은 **델리게이트만 구독**하고 비즈니스 로직 없음

- [ ] **Step 1:** 채널 선택(3옵션) + `EditableText` + 전송 버튼.
- [ ] **Step 2:** 수신 시 로그 영역에 채널 태그 색상 구분.
- [ ] **Step 3:** 로컬 플레이 검증.
- [ ] **Step 4:** Commit — `feat(client): minimal chat UI`

---

### Task 9: 통합 검증·문서

- [ ] **Step 1:** Gateway + World + Realm + Client 동시 기동 (`Shared/BuildScripts/StartServers.bat` 등) 후 2클라 이상으로 LOCAL / WORLD 확인. **렐름 확성기가 서로 다른 World 인스턴스까지 도달하는지**는 World 프로세스 2개 이상(또는 스테이징)에서 검증하거나, 단일 World에서는 Realm 그룹화·전송 로그로 1차 확인 후 멀티월드에서 재검증한다.
- [ ] **Step 2:** 스펙 문서에 “구현 완료 시 확인 체크리스트” 한 절 추가(선택).
- [ ] **Step 3:** 최종 커밋 — `docs: note chat manual test steps` 또는 기능 커밋에 테스트 방법을 본문에 포함.

---

## 테스트 전략 (코드베이스 현실 반영)

- 서버에 GTest 등이 없으면 **Task 3~5**는 “빌드 성공 + 로그 기반 수동 검증”을 통과 기준으로 한다.
- 이후 CI에 통합하려면 `WorldChatBroadcastService`만 링크하는 **최소 단위 테스트 프로젝트** 추가를 별 플랜으로 권장.

---

## Plan review

- **code-reviewer** 서브에이전트로 본 플랜 + 스펙 교차 검토 완료. 위 **보안·신뢰 경계**, **Gateway 릴레이 검증**, **중복 world 세션**, **WORLD 수신자 소스**, **멀티월드 REALM 검증** 문단이 그 결과를 반영한 수정이다.
- 선택: `plan-document-reviewer`로 한 번 더 돌려도 됨(정책상 요구 시).

---

## 실행 위임

플랜 검토 통과 후:

**Plan complete and saved to `docs/superpowers/plans/2026-03-31-in-game-chat.md`. Two execution options:**

1. **Subagent-Driven (recommended)** — 태스크마다 새 서브에이전트, 태스크 간 리뷰  
2. **Inline Execution** — 본 세션에서 `executing-plans`로 체크포인트마다 실행

**Which approach?** (구현 시작 시 선택)
