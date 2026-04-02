# PacketGenerator 코드 정리 설계 스펙

## 개요

`Shared/Tools/PacketGenerator/` .NET 10 콘솔 앱의 코드 중복 제거, 유지보수성 개선, 데드 코드 정리를 수행한다.
코드 리뷰에서 발견된 16개 이슈를 4단계 안전 리팩터링으로 해결한다.

## 배경

PacketGenerator는 protobuf descriptor 파일(.desc)을 읽어 C++ 패킷 핸들러 헤더를 생성하는 도구이다.
현재 5개 C# 파일(~807줄) + 빌드 스크립트(PacketGenerator.bat)로 구성된다.

**현재 문제점:**
- `PacketHandlerGenerator.cs`에 5개 무관한 클래스가 337줄에 몰려 있음
- `PacketId.h`가 `Program.cs`와 `PacketGenerator.bat`에서 이중 복사됨
- `PacketConfig.Load`와 `Program.cs`에서 에러 핸들링이 이중화되어, 설정 파일 오류 시 exit code 0(성공)으로 종료
- `AUTO_GENERATED_WARNING` 상수가 정의되어 있으나 사용되지 않고, 동일 배너가 3개 템플릿에 인라인
- 미사용 import, 빈 상수, 누락된 `Protoc.bat` 참조 등 데드 코드 산재

## 결정사항

- **범위:** 16개 이슈 전체 수정
- **파일 구조:** 플랫 구조 (하위 폴더 없음)
- **에러 핸들링:** `PacketConfig.Load`는 예외를 throw, `Program.cs`가 유일한 catch 지점
- **리팩터링 전략:** 4단계 안전 리팩터링, 각 단계마다 빌드 검증 후 커밋

## 이슈 목록

### High (4건)

| ID | 이슈 | 파일 |
|---|---|---|
| H1 | PacketId.h 이중 복사 (bat + C#) | `PacketGenerator.bat`, `Program.cs` |
| H2 | Config load 에러 핸들링 이중화, 실패 시 exit 0 | `PacketConfig.cs`, `Program.cs` |
| H3 | 337줄 모놀리스 파일에 5개 클래스 | `PacketHandlerGenerator.cs` |
| H4 | csproj PreBuild에 존재하지 않는 `Protoc.bat` 참조 | `PacketGenerator.csproj` |

### Medium (7건)

| ID | 이슈 | 파일 |
|---|---|---|
| M1 | `AUTO_GENERATED_WARNING` 미사용, 배너 3x 인라인 | `PacketFormatter.cs` |
| M2 | CodedInputStream 파싱 패턴 중복 (파일 옵션 vs 메시지 옵션) | `PacketHandlerGenerator.cs` |
| M3 | `PacketConfig.cs` 미사용 import 4개 | `PacketConfig.cs` |
| M4 | `PacketHandlerGenerator.cs` 미사용 `using System.Data` | `PacketHandlerGenerator.cs` |
| M5 | 빈 상수 `PROTO_FILE_INCLUDE_FORMAT` | `PacketFormatter.cs` |
| M6 | `GeneratePackageOnBuild` 불필요하게 활성화 | `PacketGenerator.csproj` |
| M7 | `outputDirPath` 파라미터명이 실제로는 파일 경로 | `PacketHandlerGenerator.cs` |

### Low (5건)

| ID | 이슈 | 파일 |
|---|---|---|
| L1 | `getEnumDescriptorProto` camelCase (C# 컨벤션 위반) | `PacketHandlerGenerator.cs` |
| L2 | 이중 세미콜론 `;;` | `PacketHandlerGenerator.cs` |
| L3 | `mOptions` 헝가리안 표기법 + public 필드 | `PacketConfig.cs` |
| L4 | `SEND_MAKE_SEND_BUFFER_FORMAT` enum 스코프 불일치 | `PacketFormatter.cs`, `PacketHandlerGenerator.cs` |
| L5 | 빈 프로젝트 리스트 경고 없음 | `Program.cs` |

## 단계별 실행 계획

### 1단계: 데드 코드 정리 (위험도: 없음)

**대상 이슈:** M3, M4, M5, M6, L2

| 파일 | 변경 |
|---|---|
| `PacketConfig.cs` | `using System;`, `System.Collections`, `System.Collections.Generic`, `System.Text` 제거 |
| `PacketHandlerGenerator.cs` | `using System.Data;` 제거 |
| `PacketFormatter.cs` | `PROTO_FILE_INCLUDE_FORMAT` 상수 삭제 |
| `PacketGenerator.csproj` | `<GeneratePackageOnBuild>true</GeneratePackageOnBuild>` 삭제 |
| `PacketHandlerGenerator.cs` | line 58 `;;` → `;` |

### 2단계: 명명/스타일 통일 (위험도: 매우 낮음)

**대상 이슈:** L1, M7, L3, L4

| 파일 | 변경 |
|---|---|
| `PacketHandlerGenerator.cs` | `getEnumDescriptorProto` → `GetEnumDescriptorProto` |
| `PacketHandlerGenerator.cs` | `GenerateSharedEnum` 파라미터명 `outputDirPath` → `outputFilePath` |
| `PacketConfig.cs` | `public static mOptions` → `private static readonly JsonSerializerOptions s_jsonOptions` |
| `PacketFormatter.cs` | `RECEIVE_HANDLE_DECLARE_FORMAT`, `SEND_MAKE_SEND_BUFFER_FORMAT`의 `\r\n` → verbatim string 통일 |
| `PacketFormatter.cs` | `SEND_MAKE_SEND_BUFFER_FORMAT`: `packet_id::{1}` → `packet_id::e{0}PacketId::{1}` |
| `PacketHandlerGenerator.cs` | `Generator`에서 `SEND_MAKE_SEND_BUFFER_FORMAT` 호출 시 `handler.ProtoFileName` 전달 |

### 3단계: 중복 제거 + 에러 핸들링 (위험도: 낮음)

**대상 이슈:** M1, H1, H2, L5

| 파일 | 변경 |
|---|---|
| `PacketFormatter.cs` | 3개 템플릿(`ENUM_PACKET_ID_FORMAT`, `HANDLE_SERVICE_TYPE_FILE_FORMAT`, `HANDLE_FILE_FORMAT`)의 인라인 배너 → `AUTO_GENERATED_WARNING`를 합성하는 방식으로 교체 |
| `PacketConfig.cs` | 내부 try-catch 전부 제거. `File.Exists` 체크 제거. 파싱 실패 시 예외 그대로 throw |
| `Program.cs` | 기존 catch 블록이 유일한 에러 처리 지점. `resultConfig.Projects.Count == 0` 경고 추가 |
| `PacketGenerator.bat` | 마지막 3줄(PacketId.h xcopy) 제거 |

### 4단계: 구조 분리 (위험도: 중간)

**대상 이슈:** H3, H4

`PacketHandlerGenerator.cs`를 5개 파일로 분리:

| 새 파일 | 내용 | 예상 줄수 |
|---|---|---|
| `PacketInfo.cs` | `PacketInfo`, `HandlerInfo` 데이터 클래스 | ~25줄 |
| `RoleHelper.cs` | `RoleHelper` 역할 매칭 로직 | ~20줄 |
| `Parser.cs` | `Parser` 클래스 (descriptor 파싱) | ~160줄 |
| `EnumGenerator.cs` | `EnumGenerator` (PacketId.h 생성) | ~50줄 |
| `HandlerGenerator.cs` | 기존 `Generator` → `HandlerGenerator`로 리네임 | ~80줄 |

`PacketHandlerGenerator.cs` 삭제.

`PacketGenerator.csproj`에서 존재하지 않는 `Protoc.bat` 참조하는 PreBuild 타겟 제거.

## 최종 파일 구조

```
Shared/Tools/PacketGenerator/
  Program.cs              (에러 핸들링 통합, 빈 프로젝트 경고)
  PacketConfig.cs         (순수 파싱, throw on failure)
  PacketInfo.cs           (NEW - PacketInfo, HandlerInfo)
  RoleHelper.cs           (NEW - 역할 매칭)
  Parser.cs               (NEW - descriptor 파싱)
  EnumGenerator.cs        (NEW - PacketId.h 생성)
  HandlerGenerator.cs     (NEW - 핸들러 .h 생성)
  PacketFormatter.cs      (배너 통합, 데드코드 제거, 스타일 통일)
  ClientProtocolCopier.cs (변경 없음)
  PacketGenerator.csproj  (빌드 설정 정리, PreBuild 제거)
  PacketGenerator.sln     (변경 없음)
```

## 검증 방법

각 단계마다:
1. `dotnet build Shared/Tools/PacketGenerator/PacketGenerator.csproj -c Release` 성공 확인
2. 4단계 완료 후: `PacketGenerator.bat` 실행하여 생성된 C++ 파일을 이전 출력과 diff 비교 (2단계 L4 수정에 의한 `MakeSendBuffer` 스코프 변경 제외)

## 비범위 (Out of Scope)

- Parser 내부의 CodedInputStream 중복 패턴(M2)은 분리 단계에서 자연스럽게 개선 가능하나, 추상화를 위한 별도 헬퍼 메서드 추출은 이번 범위에서 제외. 현재 두 루프의 필드 번호와 처리 방식이 상이하여, 무리한 추상화보다 코드 가독성 유지를 우선.
- `PacketGenerator.bat`에서 `Protoc.bat` 분리 추출은 이번 범위에서 제외. 현재 bat 파일이 잘 동작하고 있으며, PreBuild 참조 제거로 정합성은 확보됨.
