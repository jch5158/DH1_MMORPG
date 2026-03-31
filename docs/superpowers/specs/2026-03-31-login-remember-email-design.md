# 로그인 폼 — 이메일 저장(체크박스) 설계

## Overview

로그인 Slate 패널(`SLoginPanel`)에 **「이메일 저장」** 체크박스를 추가한다. 사용자가 체크한 상태로 **로그인에 성공했을 때만** 이메일(정규화된 문자열)을 로컬에 저장하고, 다음 클라이언트 실행 시 이메일 입력칸과 체크 상태를 복원한다. **비밀번호는 저장하지 않는다.**

## 동작 규칙

### 기본값

- 첫 실행·설정 키가 없을 때: 체크박스 **OFF**, 이메일 칸 **빈 문자열**.

### 표시(패널 생성/표시 시)

- `GConfig`에서 `bRememberEmail`, `SavedEmail`을 읽는다.
- `bRememberEmail == true` 이고 `SavedEmail`이 비어 있지 않으면: 이메일 입력에 `SavedEmail`을 넣고 체크박스를 **ON**.
- 그 외: 이메일 칸 비움, 체크 **OFF**.

### 저장·삭제(디스크 반영)

**오직 로그인 성공 시점에만** 설정 파일을 갱신한다. (로그인 버튼 클릭 직후·HTTP 요청 전에는 쓰지 않음. 로그인 실패 시에는 읽기 전용으로 두거나 기존 저장을 유지.)

- 체크 **ON** + 로그인 성공: `SavedEmail` ← 현재 시도에 사용한 정규화 이메일, `bRememberEmail` ← `true`.
- 체크 **OFF** + 로그인 성공: `SavedEmail` 비움, `bRememberEmail` ← `false` (기존에 저장돼 있던 이메일도 **즉시 제거**).

### `ResetPanel` 등 외부에서 이메일 문자열을 넘기는 경우

- 이메일 인증/에러 복귀 등에서 `ResetPanel(Status, Email)`로 이메일이 전달되면, **그 호출의 `Email` 인자가 입력칸 표시에 우선**한다(저장된 값보다 화면 흐름이 우선).
- 저장 로직은 여전히 **로그인 성공 시점**에만 수행한다.

## 저장 위치

- **`GGameUserSettingsIni`**(일반적으로 `Saved/Config/Windows/GameUserSettings.ini` 등)에 섹션 예: **`DH1Auth`**.
- 키:
  - `bRememberEmail` — bool (`0`/`1` 또는 UE bool 문자열 관례에 맞춤)
  - `SavedEmail` — string
- 평문 INI이므로 민감 정보는 넣지 않는다(이메일만).

구현 시 `GConfig->GetBool` / `GetString`, `SetBool` / `SetString`, 마지막에 해당 INI에 `Flush(false)` 호출 패턴을 사용한다.

## UI · 코드 위치

- **`SLoginPanel`**: 체크박스 위젯(`SCheckBox`) 멤버, `Construct`에서 레이아웃(이메일/비밀번호 근처, 로그인 버튼 위 또는 아래).
- **로드**: `Construct` 끝 또는 첫 표시 시 설정 읽기 → `EmailInput` / 체크박스 반영.
- **저장**: `HandleGatewayLoginResult`(또는 실제로 “로그인 성공”이 확정되는 단일 경로)에서 위 분기 수행.

필요 시 읽기/쓰기 전용 헬퍼(예: `AuthLocalPreferences` 정적 함수 또는 작은 `namespace`)로 분리해 Slate에서 INI 직접 조작을 한곳에 모은다.

## 테스트 시나리오

1. 최초 실행: 이메일 빈칸, 체크 OFF.
2. 체크 ON → 올바른 계정으로 로그인 성공 → 재실행 시 동일 이메일·체크 ON.
3. 체크 ON 상태에서 로그인 성공 후, 체크 OFF → 다시 로그인 성공 → 재실행 시 이메일 빈칸·체크 OFF.
4. 체크 ON → **잘못된 비밀번호**로 실패 → 재실행 시 **이전에 저장된 값만** 유지(실패한 시도의 이메일로 덮어쓰지 않음).
5. `ResetPanel`으로 특정 이메일이 넘어오면 해당 문자열이 입력칸에 보이는지 확인.

## 범위 밖

- 비밀번호·토큰·자동 로그인.
- 여러 계정 프로필 전환(단일 `SavedEmail`만).

## 구현 순서 제안

1. INI 읽기/쓰기 헬퍼 + 상수(섹션명·키명).
2. `SLoginPanel` UI + Construct 로드.
3. 로그인 성공 콜백에서 저장/삭제 분기.
4. 수동 테스트 후 필요 시 `ResetPanel`과의 우선순위 미세 조정.
