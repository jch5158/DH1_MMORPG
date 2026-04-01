# 채팅 콤보 대비 + AOI 엔터티 갱신 — 설계

**날짜:** 2026-04-01  
**상태:** 초안 (구현 전 검토용)

---

## 1. 문제 정의

### 1.1 채팅 채널 콤보 (일반 / 월드 / 렐름)

- **증상:** 드롭다운 항목이 **흰 배경에 밝은 글자**로 보여 읽기 어렵다.
- **맥락:** `UClientNetSubsystem::RegisterChatUi`에서 `OnGenerateWidgetEvent`로 `UBorder` + `UTextBlock`을 생성한다 (`HandleChatComboGenerateItem`). `ApplyDarkChatComboDropdownStyle`으로 `FTableRowStyle`(행 배경·텍스트)은 지정하지만, **커스텀 위젯 행은 기본 `ItemStyle` 브러시가 행 전체에 자동 적용되지 않을 수 있고**, `UBorder`에 **배경 브러시/틴트를 설정하지 않으면** 엔진 기본(밝은 톤)이 쓰일 수 있다. 텍스트 색은 `FSlateColor(0.92, 0.95, 1.0)`로 **거의 흰색**에 가깝다.
- **정리:** 단일 원인 단정은 피하고, **(a)** 리스트가 커스텀 서브트리 뒤를 어둡게 칠하지 않음 **(b)** `UBorder` 기본·투명 브러시 **(c)** 밝은 글자색이 겹친 결과로 본다. 최종 확인은 **PIE 시각 검사**(필요 시 위젯 리플렉터/Slate 디버거). `UComboBoxString` 구현에 따라 **행 vs 드롭다운 패널**이 별 레이어일 수 있다.

### 1.2 영향권(AOI) 셀 — 주변 오브젝트 미갱신

- **증상:** 셀/가시 범위가 바뀌어도 클라이언트에 원격 엔터티가 나타나지 않거나 갱신되지 않는다.
- **맥락 (코드 기준):**
  - **서버:** `GameTickProcessor::processVisibilityChanges`는 **그리드 셀 ID가 바뀔 때만** 플레이어에게 주변 `S2C_ENTITY_ENTER_NOT`를 보낸다. 스폰 직후·동일 셀 정체 시에는 `NotifySpawnedPlayerVisibleEntities` / `NotifyNearbyPlayersAboutEntity`에 의존한다.
  - **클라:** `MovementPacketHandler`가 ENTER/SNAPSHOT/LEAVE를 게임 스레드에서 `ApplyNetworkEntitiesEntered` 등으로 넘긴다.
  - **잠재 원인 후보:** (A) `UGameInstanceSubsystem::GetWorld()`가 호출 시점에 null → 스폰 스킵 (시점: **시암리스 트래블, PIE 다중 클라, 연결 직후 첫 프레임** 등과 상관 있으면 로그에 적어 둠). (B) 월드→게이트웨이 릴레이(`sendRelayToClient` / `S2S_RELAY_TO_CLIENT_NOT`) **gatewaySessionId** vs 게이트웨이 `GetSessionBySessionId` 기대값 불일치. (C) 서버가 ENTER를 보내지 않는 경로(스폰 알림 누락, `GetObjectsInRange` 범위 0). (D) 클라 **패킷 서비스 타입/opcode → 핸들러 등록** 오류 또는 조기 return. (E) `hasWorld:1`·서버 릴레이 정상인데도 미표시 → **클라 디스패치·중복 ENTER 무시 맵** 등 앱 로직.

---

## 2. 접근 방식 비교 (2~3안)

| 안 | 채팅 콤보 | AOI | 장단점 |
|----|-----------|-----|--------|
| **A (권장)** | `UBorder`에 어두운 `Background` / `BrushColor` + 글자색 명시 대비 | 런타임 로그(`debug-*.log` 등)로 **H2: GetWorld**, 서버 로그/패킷 카운터로 릴레이·ENTER 발생 여부 확인 후 **입증된 경로만** 수정 | 증거 기반, 부작용 최소 |
| **B** | WBP에서 콤보 스타일만 에디터로 지정, C++ 생성기 제거 | 서버에 광범위 로그·강제 풀 스냅샷 틱 추가 | 유지보수·부하 부담 |
| **C** | Slate `SComboBox` 전부 커스텀 | 클라만 폴링으로 월드 쿼리 | 범위 과대, YAGNI |

**권장:** **안 A** — 콤보는 Border 배경·텍스트 대비를 코드 한 곳에서 고치고, AOI는 기존 계측/로그로 가설을 CONFIRMED/REJECTED 한 뒤 해당 레이어만 수정한다.

---

## 3. 설계 — 채팅 콤보

- **목표:** 드롭다운 **각 행**이 어두운 배경 + 밝지 않은 본문 색(또는 밝은 글자 + 확실한 어두운 배경)으로 **WCAG에 가깝게** 읽힌다.
- **구현 요지:**
  - `HandleChatComboGenerateItem` 내 `UBorder`에 **`Background`(`FSlateBrush`) + 틴트** 또는 `BrushColor` 등 엔진 API에 맞는 방식으로 **불투명 어두운 배경** 지정 — **틴트만 쓸 때는 브러시가 실질적으로 채움을 그리는지(알파)** 확인 (빈 브러시에 틴트만 주면 여전히 밝게 보일 수 있음).
  - `UTextBlock` 색은 배경과 **명도 차** 확보 (예: 배경 0.1대면 글자 0.9대 유지 가능, 배경이 밝아지면 글자를 어둡게).
  - (선택) 호버 시 `Border` 틴트 살짝 밝게 — UMG 이벤트 또는 스타일 테이블.
  - 행만 고쳐도 주변이 하얗면 **`ComboBox`의 `WidgetStyle`(드롭다운 패널/메뉴 보더)** 를 한 단계 더 조정하는 검증만 추가 (YAGNI — 증상이 남을 때만).
- **검증:** PIE에서 **닫힌 콤보 버튼**과 **펼친 목록 행**을 각각 확인, 세 옵션 가독성·스크린샷 before/after.

---

## 4. 설계 — AOI 갱신

- **목표:** 서버가 보내는 ENTER/SNAPSHOT/LEAVE가 클라에서 **일관되게** 반영되고, `GetWorld()` 등 **컨텍스트 버그**가 있으면 제거한다.
- **구현 요지 (증거 후):**
  - **클라:** `ApplyNetworkEntitiesEntered`에서 `GetWorld()`가 null이면 `GEngine` + `WorldContexts`에서 **현재 게임 월드**를 한 번 더 해석하는 보조 경로는 **로그로 H2가 CONFIRMED된 경우에만** 추가 (추측용 가드 누적 금지).
  - **서버:** `NotifySpawnedPlayerVisibleEntities` / `NotifyNearbyPlayersAboutEntity` 호출 지점과 `sendRelayToClient`의 `gatewaySessionId`·`gatewayServerId`가 유효한지 로그로 확인.
  - **스냅샷:** `broadcastSnapshots`가 `!pNearby->IsMoving()`인 엔터티를 건너뛰므로, **정지한 원격 엔터티**는 스냅샷으로 안 올 수 있음 — ENTER가 반드시 선행되는지 플랜에서 검증 항목으로 둔다.
- **검증:** 클라 2개 + 서버, 이동 후 큐브(프록시) 등장/이동/사라짐, `debug-ab4bf2.log` 또는 동등 NDJSON에서 `hasWorld:1`, `batch>0` 확인.

---

## 5. 테스트·완료 기준

- [ ] 콤보: 세 채널 이름이 드롭다운에서 **명확히 읽힘** (사용자 확인).
- [ ] AOI (가능한 맵/시나리오에서 **아래를 모두** 시도; 특정 항목이 구조상 N/A면 플랜에 한 줄 사유 기록):
  - [ ] **ENTER →** 원격 프록시(또는 대체 시각화) 스폰.
  - [ ] **LEAVE →** 제거.
  - [ ] **원격 이동 →** 위치 갱신(또는 스냅샷/ENTER 재수신).
- [ ] 디버그 계측: 성공 확인 후 사용자 요청 시에만 제거 (기존 디버그 모드 정책).

---

## 6. 승인

이 문서가 범위·접근에 맞으면 **구현 플랜**(`docs/superpowers/plans/2026-04-01-chat-combo-contrast-aoi-update-plan.md`)으로 태스크를 쪼갠다. 수정 요청이 있으면 본 스펙을 먼저 갱신한다.
