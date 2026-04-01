# 인게임 채팅 설계 (일반 / 월드 / 렐름 확성기)

## 목표 — 채널 3종

| 채널 | 한글 명칭(예시) | 수신 범위 |
|------|-----------------|-----------|
| **일반** | 근처, 로컬 | **AOI** — 발신자 기준 `GridManager::GetObjectsInRange(cellX, cellY)` 에 들어오는 **플레이어만** (엔티티 스냅샷과 동일한 셀 반경 `mAoiRange`). |
| **월드** | 맵(월드) | 동일 **WorldServer 인스턴스**에 접속한 **전원** (모든 `PlayerObject` / `GameSessionManager` 기준 세션). |
| **렐름** | 확성기 | 동일 **Realm**에 등록된 온라인 **전원** (여러 World·Gateway 팬아웃). |

비목표(초기 버전): 귓속말, 길드 채널, 욕설 필터·영구 로그 DB, **확성기 소비 아이템/쿨다운**(추후 기획 연동 시 `C2S_CHAT_REQ` 검증 단계에서 확장).

## 현재 아키텍처와 제약

- 클라이언트 ↔ **Gateway** ↔ **World**; `S2S_RELAY_TO_WORLD_NOT` / `S2S_RELAY_TO_CLIENT_NOT`.
- **Realm**은 World와 TCP; `RealmSessionRegistry`에 `(accountId → worldServerId, gatewayServerId)`.
- Realm은 클라이언트에 직접 송신 불가 → **렐름(확성기)** 은 Realm이 월드별로 묶어 `S2S_REALM_CHAT_DELIVER_NOT` 후 각 World가 Gateway로 릴레이.

## AOI(일반 채팅) 구현 기준

- 발신 `PlayerObject`의 현재 위치로 `cellX/cellY` 계산 후 `GridManager::GetObjectsInRange(cellX, cellY)` 사용.
- 결과 중 `eGameObjectType::Player` 만 수신 대상; 발신자 본인 포함(에코는 UI 정책에 따름, 서버는 동일 payload로내도 됨).
- 스냅샷 브로드캐스트(`GameTickProcessor::broadcastSnapshots`)와 **같은 그리드·반경**을 쓰므로 “보이는 사람 = 채팅 받는 사람”이 맞다.

## 프로토콜

### `Enum.proto`

- `SERVICE_TYPE_CHAT = 8`
- `eChatChannel`: `CHAT_CHANNEL_LOCAL = 0` (일반/AOI), `CHAT_CHANNEL_WORLD = 1`, `CHAT_CHANNEL_REALM = 2` (확성기)

### `Chat.proto` (신규)

**클라 → Gateway → World**

- `C2S_CHAT_REQ`: `eChatChannel channel`, `string message` (UTF-8, 길이 상한·개행 정책 서버 검증)

**World → 클라 (릴레이 payload)**

- `S2C_CHAT_NOT`: `eChatChannel channel`, `uint64 sender_account_id`, `string sender_display_name`, `string message`, `int64 server_timestamp_ms` (권장)

**World → Realm (렐름 채널만)**

- `S2S_REALM_CHAT_SUBMIT_NOT`: 송신자 정보 + 메시지 + 타임스탬프; `channel` 필드는 `CHAT_CHANNEL_REALM` 고정이거나 enum으로 명시

**Realm → World**

- `S2S_REALM_CHAT_DELIVER_NOT`: `bytes s2c_payload` (직렬화된 `S2C_CHAT_NOT`), `repeated RelayTarget { uint64 gateway_session_id; int32 gateway_server_id; }`

**월드·일반 채널**은 Realm을 거치지 않는다.

### Gateway

- Movement와 동일하게 `S2S_RELAY_TO_WORLD_NOT` payload에 Chat 패킷 실어 `ChatPacketHandler` 디스패치.

## 서버 로직 요약

### WorldServer

1. 공통 검증: 세션·플레이어 존재, UTF-8·길이, 레이트 리밋(채널별 상이).
2. **LOCAL**: 송신자 셀 기준 `GetObjectsInRange` → 플레이어만 `sendRelayToClient`.
3. **WORLD**: `GetAllObjects()` 또는 `GameSessionManager` 순회로 모든 플레이어에 릴레이.
4. **REALM**: `S2C_CHAT_NOT` 직렬화 후 `S2S_REALM_CHAT_SUBMIT_NOT` → Realm.

### RealmServer

- `S2S_REALM_CHAT_SUBMIT_NOT` 수신 → 레지스트리 팬아웃 → 월드별 `S2S_REALM_CHAT_DELIVER_NOT`.

### WorldServer (Realm에서 수신)

- `HANDLE_S2S_REALM_CHAT_DELIVER_NOT` → 각 타깃에 `S2S_RELAY_TO_CLIENT_NOT`.

## 보안·운영

- **레이트 리밋 권장**: `LOCAL` ≥ `WORLD` ≥ `REALM`(확성기) 순으로 엄격 — 확성기는 전체 브로드캐스트이므로 가장 빡세게 (예: 분당 N회·계정 단위).
- 스팸·어뷰 방지: 동일 메시지 연속 차단, 최소 간격 등은 World 공통 처리 가능.

## 클라이언트 (UE)

- `S2C_CHAT_NOT` 수신 시 `channel`으로 탭·색상 구분(일반 / 월드 / [확성기] 등).
- 입력 UI에서 채널 선택 후 `C2S_CHAT_REQ` 전송.

## 구현 순서 제안

1. Proto + 패킷 생성 + Gateway/World 릴레이 등록  
2. **LOCAL** (AOI)만 먼저 검증  
3. **WORLD** 전원 브로드캐스트  
4. **REALM** Submit/Deliver + Realm 라우팅  
5. 클라 UI·레이트·로그

---

스펙 확정 후 `writing-plans`로 구현 플랜 분해.
