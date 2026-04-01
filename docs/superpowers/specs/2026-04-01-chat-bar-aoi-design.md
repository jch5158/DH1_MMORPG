# Chat bar layout + AOI visibility — design (2026-04-01)

## Problem

1. **Chat bar:** `WBP_Chat` 하단 행에서 `Combo_Channel`(콤보), `EditableText_Message`, `Btn_Send`의 **세로 높이·폰트 두께·패딩**이 서로 다르고, 콤보 **선택 전후** Slate가 다시 그려지며 레이아웃이 어긋난다.
2. **AOI:** 서버 영향권/스냅샷/ENTER 수정 후에도 클라이언트에서 **다른 플레이어 프록시가 보이지 않는다**는 보고가 이어진다.

## Approach

### Chat UI (C++-only, no BP binary edit)

- 단일 소스: `UComboBoxString::GetFont()`를 **입력란 `FEditableTextBoxStyle::TextStyle`** 및 (가능 시) 버튼 직계 자식 `UTextBlock`에 적용.
- 단일 세로 패딩: `ApplyLightChatComboDropdownStyle`의 `ContentPadding` / `ComboButtonStyle.ContentPadding` / `SetContentPadding` / `HandleChatComboGenerateItem`의 `UBorder` 패딩 / `EditableTextBox`의 `Padding`을 **동일한 `FMargin(10,5,10,5)`**로 맞춤.
- `UHorizontalBoxSlot::SetVerticalAlignment(VAlign_Center)`로 콤보·입력·버튼을 **한 줄 세로 중앙** 정렬.

Blueprint `WBP_Chat`의 슬롯 규칙(Fill/Auto)은 유지; 폰트·패딩·정렬만 런타임에서 통일한다.

### AOI

- **서버:** `broadcastSnapshots`의 정지 플레이어 포함, `processVisibilityChanges`에서 플레이어 셀 전환 시 `NotifyNearbyPlayersAboutEntity` 호출은 이미 반영됨. 배포 **WorldServer 바이너리 재빌드·재시작**이 전제.
- **게이트웨이:** `S2S_RELAY_TO_CLIENT_NOT`는 `gatewaySessionId`로 클라 세션을 찾아 payload를 그대로 전달. 세션 미스매치 시 Gateway 로그에 `Client session not found for relay`가 남는다.
- **클라:** `debug-ab4bf2.log`의 `MovementPacketHandler:SNAPSHOT_gt`(`batch`, `firstEntityId`), `ClientNetSubsystem:ApplyNetworkEntitiesEntered`로 패킷 도달 여부를 확인.
- **좌표:** 서버→클라 위치가 월드와 다른 축 규약이면 큐브가 화면 밖에 생길 수 있음 — 로그에 샘플 좌표를 남겨 검증.

## Out of scope (follow-up)

- `WBP_Chat`에서 `USizeBox`로 고정 높이를 주는 비주얼 폴리시(에디터 작업).
- AOI 좌표 변환 버그가 로그로 확인되면 별도 수정.

## Approval

이 스펙은 구현과 플랜 문서화에 사용한다.
