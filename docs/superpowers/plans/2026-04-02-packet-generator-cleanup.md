# PacketGenerator 코드 정리 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** PacketGenerator .NET 도구의 코드 중복, 데드 코드, 명명 불일치, 구조 문제를 19건 수정하여 유지보수성을 개선한다.

**Architecture:** 4단계 안전 리팩터링 — 데드 코드 정리 → 명명/스타일 → 중복 제거 + 에러 핸들링 → 구조 분리. 각 단계마다 `dotnet build`로 빌드 검증 후 커밋.

**Tech Stack:** .NET 10, C#, Google.Protobuf, PowerShell

**Spec:** `docs/superpowers/specs/2026-04-02-packet-generator-cleanup-design.md`

---

## File Map

| File | Action | Responsibility |
|---|---|---|
| `Shared/Tools/PacketGenerator/PacketConfig.cs` | Modify | JSON config 파싱 (throw on failure) |
| `Shared/Tools/PacketGenerator/PacketHandlerGenerator.cs` | Delete (Phase 4) | 5개 클래스 → 분리 후 삭제 |
| `Shared/Tools/PacketGenerator/PacketFormatter.cs` | Modify | C++ 템플릿 문자열 |
| `Shared/Tools/PacketGenerator/PacketGenerator.csproj` | Modify | 빌드 설정 |
| `Shared/Tools/PacketGenerator/Program.cs` | Modify | 진입점, 에러 핸들링 |
| `Shared/Tools/PacketGenerator/PacketInfo.cs` | Create (Phase 4) | PacketInfo, HandlerInfo 데이터 클래스 |
| `Shared/Tools/PacketGenerator/RoleHelper.cs` | Create (Phase 4) | 역할 매칭 로직 |
| `Shared/Tools/PacketGenerator/Parser.cs` | Create (Phase 4) | Descriptor 파싱 |
| `Shared/Tools/PacketGenerator/EnumGenerator.cs` | Create (Phase 4) | PacketId.h 생성 |
| `Shared/Tools/PacketGenerator/HandlerGenerator.cs` | Create (Phase 4) | 핸들러 .h 생성 |
| `Shared/BuildScripts/PacketGenerator.bat` | Modify (Phase 3) | 빌드 스크립트 |

---

## Task 1: 생성 파일 스냅샷 저장

**Files:**
- No file changes

- [ ] **Step 1: 현재 생성 파일 스냅샷 저장**

회귀 비교 기준으로 사용할 스냅샷을 생성한다.

```powershell
cd E:\Projects\DH1_MMORPG
$dirs = @(
    "DH1_Engine\EchoClient\PacketHandler",
    "DH1_Engine\EchoServer\PacketHandler",
    "DH1_Client\Source\DH1_Client\Network\PacketHandler",
    "DH1_Server\GatewayServer\PacketHandler",
    "DH1_Server\WorldServer\PacketHandler",
    "DH1_Server\RealmServer\PacketHandler",
    "Shared\Protocol\PacketId"
)
New-Item -ItemType Directory -Force -Path ".snapshot"
foreach ($d in $dirs) {
    $dest = ".snapshot\$($d -replace '\\','_')"
    Copy-Item -Path $d -Destination $dest -Recurse -Force
}
```

Expected: `.snapshot/` 디렉토리에 7개 하위 폴더 생성됨

- [ ] **Step 2: 루트 `.gitignore`에 .snapshot 추가**

`E:\Projects\DH1_MMORPG\.gitignore` 파일 끝에 추가하여 `.snapshot/` 폴더가 커밋되지 않도록 한다:

```
# 끝에 추가
.snapshot/
```

---

## Task 2: Phase 1 — 데드 코드 정리

**Files:**
- Modify: `Shared/Tools/PacketGenerator/PacketConfig.cs`
- Modify: `Shared/Tools/PacketGenerator/PacketHandlerGenerator.cs`
- Modify: `Shared/Tools/PacketGenerator/PacketFormatter.cs`
- Modify: `Shared/Tools/PacketGenerator/PacketGenerator.csproj`

- [ ] **Step 1: `PacketConfig.cs` 미사용 import 제거**

파일 상단에서 아래 5줄을 삭제한다:

```csharp
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
```

그리고 `using JetBrains.Annotations;` 도 삭제한다.

수정 후 파일 상단은 다음만 남아야 한다:

```csharp
using System.Text.Json;

namespace PacketGenerator
{
```

- [ ] **Step 2: `PacketHandlerGenerator.cs` 미사용 import 제거 + 세미콜론 수정**

`using System.Data;` (line 3) 삭제.

line 58의 이중 세미콜론 수정:

```csharp
// Before:
var roleDescriptor = getEnumDescriptorProto("eRole", protocolDirPath); ;

// After:
var roleDescriptor = getEnumDescriptorProto("eRole", protocolDirPath);
```

- [ ] **Step 3: `PacketFormatter.cs` 빈 상수 삭제**

`PROTO_FILE_INCLUDE_FORMAT` 상수(line 243)를 삭제한다:

```csharp
// 이 줄 삭제:
public static readonly string PROTO_FILE_INCLUDE_FORMAT = "";
```

- [ ] **Step 4: `PacketGenerator.csproj` 빌드 설정 정리**

아래 3개 항목을 삭제한다:

```xml
<!-- 삭제: -->
<GeneratePackageOnBuild>true</GeneratePackageOnBuild>
<ProduceReferenceAssembly>true</ProduceReferenceAssembly>
```

`JetBrains.Annotations` 패키지 참조 삭제:

```xml
<!-- 삭제: -->
<PackageReference Include="JetBrains.Annotations" Version="2025.2.4" />
```

- [ ] **Step 5: 빌드 검증**

```powershell
dotnet build Shared/Tools/PacketGenerator/PacketGenerator.csproj -c Release
```

Expected: `Build succeeded. 0 Warning(s) 0 Error(s)`

- [ ] **Step 6: 커밋**

```powershell
git add Shared/Tools/PacketGenerator/PacketConfig.cs Shared/Tools/PacketGenerator/PacketHandlerGenerator.cs Shared/Tools/PacketGenerator/PacketFormatter.cs Shared/Tools/PacketGenerator/PacketGenerator.csproj
git commit -m "refactor(PacketGenerator): remove dead code, unused imports, and unnecessary build settings"
```

---

## Task 3: Phase 2 — 명명/스타일 통일

**Files:**
- Modify: `Shared/Tools/PacketGenerator/PacketHandlerGenerator.cs`
- Modify: `Shared/Tools/PacketGenerator/PacketConfig.cs`
- Modify: `Shared/Tools/PacketGenerator/PacketFormatter.cs`

- [ ] **Step 1: `PacketHandlerGenerator.cs` 메서드명 PascalCase 수정**

`getEnumDescriptorProto` → `GetEnumDescriptorProto` (정의부 + 호출부 2곳 모두 변경)

```csharp
// 정의부 (line 195):
private static EnumDescriptorProto? GetEnumDescriptorProto(string enumName, string protocolDirPath)

// 호출부 1 (line 57):
var serviceTypeDescriptor = GetEnumDescriptorProto("eServiceType", protocolDirPath);

// 호출부 2 (line 58):
var roleDescriptor = GetEnumDescriptorProto("eRole", protocolDirPath);
```

- [ ] **Step 2: `PacketHandlerGenerator.cs` 파라미터명 수정**

`EnumGenerator.GenerateSharedEnum`의 파라미터명 변경:

```csharp
// Before:
public static void GenerateSharedEnum(List<HandlerInfo> handlers, string outputDirPath)

// After:
public static void GenerateSharedEnum(List<HandlerInfo> handlers, string outputFilePath)
```

메서드 내부의 `outputDirPath` 참조도 모두 `outputFilePath`로 변경한다 (3곳):
- `var directoryPath = Path.GetDirectoryName(outputFilePath);`
- `File.WriteAllText(outputFilePath, ...)`

- [ ] **Step 3: `PacketConfig.cs` 필드 명명 수정**

```csharp
// Before:
public static JsonSerializerOptions mOptions = new JsonSerializerOptions
{
    PropertyNameCaseInsensitive = true
};

// After:
private static readonly JsonSerializerOptions s_jsonOptions = new()
{
    PropertyNameCaseInsensitive = true
};
```

`Load` 메서드 내부의 참조도 변경:

```csharp
// Before:
var config = JsonSerializer.Deserialize<PacketProjectsConfig>(jsonString, mOptions);

// After:
var config = JsonSerializer.Deserialize<PacketProjectsConfig>(jsonString, s_jsonOptions);
```

- [ ] **Step 4: `PacketFormatter.cs` 줄바꿈 통일 + L4 enum 스코프 수정**

`RECEIVE_HANDLE_DECLARE_FORMAT`을 verbatim string으로 변경:

```csharp
// Before:
public static readonly string RECEIVE_HANDLE_DECLARE_FORMAT =
    "static bool HANDLE_{0}(const Protocol::{0}& packet, const PacketSessionRef& pSession);\r\n\t";

// After:
public static readonly string RECEIVE_HANDLE_DECLARE_FORMAT =
    @"static bool HANDLE_{0}(const Protocol::{0}& packet, const PacketSessionRef& pSession);
	";
```

`SEND_MAKE_SEND_BUFFER_FORMAT`을 verbatim string으로 변경하고 `{2}` placeholder 추가:

```csharp
// Before:
public static readonly string SEND_MAKE_SEND_BUFFER_FORMAT =
    "static NetSendBufferRef MakeSendBuffer(const Protocol::{0}& packet) {{ return MakeSendBuffer(packet, packet_id::{1}); }}\r\n\t";

// After:
public static readonly string SEND_MAKE_SEND_BUFFER_FORMAT =
    @"static NetSendBufferRef MakeSendBuffer(const Protocol::{0}& packet) {{ return MakeSendBuffer(packet, packet_id::e{2}PacketId::{1}); }}
	";
```

- [ ] **Step 5: `PacketHandlerGenerator.cs` — `SEND_MAKE_SEND_BUFFER_FORMAT` 호출부 수정**

`Generator.GenerateCpps` 메서드 내부:

```csharp
// Before:
makeSendBufferBuilder.AppendFormat(PacketFormatter.SEND_MAKE_SEND_BUFFER_FORMAT,
    packet.MessageName,
    packet.MessageName);

// After:
makeSendBufferBuilder.AppendFormat(PacketFormatter.SEND_MAKE_SEND_BUFFER_FORMAT,
    packet.MessageName,
    packet.MessageName,
    handler.ProtoFileName);
```

- [ ] **Step 6: 빌드 검증**

```powershell
dotnet build Shared/Tools/PacketGenerator/PacketGenerator.csproj -c Release
```

Expected: `Build succeeded. 0 Warning(s) 0 Error(s)`

- [ ] **Step 7: 커밋**

```powershell
git add Shared/Tools/PacketGenerator/PacketHandlerGenerator.cs Shared/Tools/PacketGenerator/PacketConfig.cs Shared/Tools/PacketGenerator/PacketFormatter.cs
git commit -m "refactor(PacketGenerator): unify naming conventions and fix SEND format enum scope"
```

---

## Task 4: Phase 3 — 중복 제거 + 에러 핸들링

**Files:**
- Modify: `Shared/Tools/PacketGenerator/PacketFormatter.cs`
- Modify: `Shared/Tools/PacketGenerator/PacketConfig.cs`
- Modify: `Shared/Tools/PacketGenerator/Program.cs`
- Modify: `Shared/BuildScripts/PacketGenerator.bat`

- [ ] **Step 1: `PacketFormatter.cs` — AUTO_GENERATED_WARNING 배너 통합**

3개 템플릿 상수에서 인라인된 배너를 제거하고 `AUTO_GENERATED_WARNING`과 합성하도록 변경한다.

`ENUM_PACKET_ID_FORMAT`:

```csharp
// Before:
public static readonly string ENUM_PACKET_ID_FORMAT =
    @"// =============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT
// This file is generated by PacketGenerator. Any manual changes will be lost.
// =============================================================================
#pragma once

namespace packet_id
{{{0}}}";

// After:
public static readonly string ENUM_PACKET_ID_FORMAT =
    AUTO_GENERATED_WARNING +
    @"#pragma once

namespace packet_id
{{{0}}}";
```

`HANDLE_SERVICE_TYPE_FILE_FORMAT`:

```csharp
// Before:
public static readonly string HANDLE_SERVICE_TYPE_FILE_FORMAT =
    @"// =============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT
// This file is generated by PacketGenerator. Any manual changes will be lost.
// =============================================================================
#pragma once
{0}
#include ""SessionValidator.h""
...

// After:
public static readonly string HANDLE_SERVICE_TYPE_FILE_FORMAT =
    AUTO_GENERATED_WARNING +
    @"#pragma once
{0}
#include ""SessionValidator.h""
...
```

`HANDLE_FILE_FORMAT`:

```csharp
// Before:
public static readonly string HANDLE_FILE_FORMAT =
    @"// =============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT
// This file is generated by PacketGenerator. Any manual changes will be lost.
// =============================================================================
// ReSharper disable CppInconsistentNaming
#pragma once
...

// After:
public static readonly string HANDLE_FILE_FORMAT =
    AUTO_GENERATED_WARNING +
    @"// ReSharper disable CppInconsistentNaming
#pragma once
...
```

세 템플릿 모두 동일한 패턴: `@"` 시작 부분의 배너 4줄 제거 → `AUTO_GENERATED_WARNING +` 접두사로 교체. 배너 직후의 나머지 내용은 그대로 유지. `NormalizeToCrlf`가 최종 출력에 적용되므로 줄바꿈은 자동 정규화됨.

- [ ] **Step 2: `PacketConfig.cs` — 에러 핸들링 단순화**

`PacketConfig` 클래스의 `Load` 메서드만 변경한다. 같은 파일의 `ProjectConfig`, `PacketProjectsConfig` 클래스는 그대로 유지.

```csharp
// Before: Load 내부에 File.Exists 체크 + try-catch 3종이 있음
// After: 모든 예외를 호출자에게 위임 (FileNotFoundException, JsonException, ArgumentNullException 등)
internal class PacketConfig
{
    public static PacketProjectsConfig Load(string configFilePath)
    {
        var jsonString = File.ReadAllText(configFilePath);
        var config = JsonSerializer.Deserialize<PacketProjectsConfig>(jsonString, s_jsonOptions);
        return config ?? throw new InvalidOperationException(
            $"JSON deserialization returned null for: {configFilePath}");
    }

    private static readonly JsonSerializerOptions s_jsonOptions = new()
    {
        PropertyNameCaseInsensitive = true
    };
}
```

- [ ] **Step 3: `Program.cs` — 빈 프로젝트 경고 추가**

`foreach` 루프 앞에 경고 추가:

```csharp
// resultConfig.Projects 루프 직전에 추가:
if (resultConfig.Projects.Count == 0)
{
    Console.WriteLine("[Warning] No projects configured in config file — nothing to generate.");
}
```

- [ ] **Step 4: `PacketGenerator.bat` — PacketId.h 이중 복사 제거**

line 53~58 (6줄)을 삭제한다:

```batch
:: 이 6줄 삭제:
set "PACKET_ID_PATH=%BASE_DIR%..\Protocol\PacketId"
set "DH1_CLIENT_PATH=%ARG_BASE_PRJ%\DH1_Client\Source\DH1_Client\Network\Protocol\PacketId"

if not exist "%DH1_CLIENT_PATH%" mkdir "%DH1_CLIENT_PATH%"

xcopy /S /Y "%PACKET_ID_PATH%\PacketId.h" "%DH1_CLIENT_PATH%\"
```

`Program.cs`의 `ClientProtocolCopier` 호출 이후 PacketId.h 복사 코드가 이미 이 역할을 수행하고 있으므로, bat의 중복 복사는 불필요.

- [ ] **Step 5: 빌드 검증**

```powershell
dotnet build Shared/Tools/PacketGenerator/PacketGenerator.csproj -c Release
```

Expected: `Build succeeded. 0 Warning(s) 0 Error(s)`

- [ ] **Step 6: 커밋**

```powershell
git add Shared/Tools/PacketGenerator/PacketFormatter.cs Shared/Tools/PacketGenerator/PacketConfig.cs Shared/Tools/PacketGenerator/Program.cs Shared/BuildScripts/PacketGenerator.bat
git commit -m "refactor(PacketGenerator): consolidate error handling, remove duplicate banner and PacketId.h copy"
```

---

## Task 5: Phase 4 — 구조 분리

**Files:**
- Create: `Shared/Tools/PacketGenerator/PacketInfo.cs`
- Create: `Shared/Tools/PacketGenerator/RoleHelper.cs`
- Create: `Shared/Tools/PacketGenerator/Parser.cs`
- Create: `Shared/Tools/PacketGenerator/EnumGenerator.cs`
- Create: `Shared/Tools/PacketGenerator/HandlerGenerator.cs`
- Delete: `Shared/Tools/PacketGenerator/PacketHandlerGenerator.cs`
- Modify: `Shared/Tools/PacketGenerator/PacketGenerator.csproj`

- [ ] **Step 1: `PacketInfo.cs` 생성**

```csharp
namespace PacketGenerator
{
    public class PacketInfo
    {
        public string MessageName { get; set; } = string.Empty;
        public uint PacketId { get; set; }
        public string Sender { get; set; } = string.Empty;
        public List<string> Receivers { get; set; } = [];
    }

    public class HandlerInfo
    {
        public string ProtoFileName { get; set; } = string.Empty;
        public string HandlerName { get; set; } = string.Empty;
        public string ServiceTypeName { get; set; } = string.Empty;
        public List<PacketInfo> Packets { get; set; } = [];
    }
}
```

- [ ] **Step 2: `RoleHelper.cs` 생성**

```csharp
namespace PacketGenerator
{
    public static class RoleHelper
    {
        private const string ServerRole = "SERVER";

        private static readonly HashSet<string> NonServerRoles = new(StringComparer.OrdinalIgnoreCase)
        {
            "CLIENT", "ECHO_CLIENT", "ECHO_SERVER", "ROLE_NONE"
        };

        public static bool IsMatchRole(string packetRole, string targetRole)
        {
            if (packetRole.Equals(ServerRole, StringComparison.OrdinalIgnoreCase))
            {
                return !NonServerRoles.Contains(targetRole);
            }

            return packetRole.Equals(targetRole, StringComparison.OrdinalIgnoreCase);
        }
    }
}
```

- [ ] **Step 3: `Parser.cs` 생성**

`PacketHandlerGenerator.cs`에서 `Parser` 클래스와 `GetEnumDescriptorProto` 메서드를 그대로 추출한다. using 구문 포함:

```csharp
using Google.Protobuf;
using Google.Protobuf.Reflection;

namespace PacketGenerator
{
    public static class Parser
    {
        // ParseHandlersFromDesc 메서드 전체 (Phase 1~2 수정 반영된 버전)
        // GetEnumDescriptorProto 메서드 전체
    }
}
```

`ParseHandlersFromDesc`와 `GetEnumDescriptorProto`의 전체 코드를 이전 단계까지의 수정사항이 반영된 상태로 그대로 옮긴다. 로직 변경 없음.

- [ ] **Step 4: `EnumGenerator.cs` 생성**

```csharp
using System.Text;

namespace PacketGenerator
{
    public static class EnumGenerator
    {
        // GenerateSharedEnum 메서드 전체 (Phase 2에서 outputFilePath로 리네임된 버전)
    }
}
```

`GenerateSharedEnum`의 전체 코드를 그대로 옮긴다. 로직 변경 없음.

- [ ] **Step 5: `HandlerGenerator.cs` 생성**

기존 `Generator` 클래스를 `HandlerGenerator`로 리네임하여 추출:

```csharp
using System.Text;

namespace PacketGenerator
{
    public static class HandlerGenerator
    {
        // GenerateCpps 메서드 전체 (Phase 2에서 SEND 포맷 3인자 호출로 변경된 버전)
    }
}
```

- [ ] **Step 6: `Program.cs` — Generator 참조를 HandlerGenerator로 변경**

```csharp
// Before:
Generator.GenerateCpps(handlers, prjConfig.Role, outputPath);

// After:
HandlerGenerator.GenerateCpps(handlers, prjConfig.Role, outputPath);
```

- [ ] **Step 7: `PacketHandlerGenerator.cs` 삭제**

5개 신규 파일로 모든 클래스가 이동되었으므로, 원본 파일을 삭제한다.

```powershell
Remove-Item Shared/Tools/PacketGenerator/PacketHandlerGenerator.cs
```

- [ ] **Step 8: `PacketGenerator.csproj` — PreBuild 타겟 제거**

존재하지 않는 `Protoc.bat` 참조를 제거한다:

```xml
<!-- 이 블록 전체 삭제: -->
<Target Name="PreBuild" BeforeTargets="PreBuildEvent" Condition="'$(SolutionDir)' != '' AND '$(SolutionDir)' != '*Undefined*'">
    <Exec Command="call &quot;$(SolutionDir)..\..\BuildScripts\Protoc.bat&quot;" />
</Target>
```

- [ ] **Step 9: 빌드 검증**

```powershell
dotnet build Shared/Tools/PacketGenerator/PacketGenerator.csproj -c Release
```

Expected: `Build succeeded. 0 Warning(s) 0 Error(s)`

- [ ] **Step 10: 커밋**

```powershell
git add Shared/Tools/PacketGenerator/
git commit -m "refactor(PacketGenerator): split monolith into focused files, remove dead PreBuild target"
```

---

## Task 6: 회귀 검증

**Files:**
- No file changes

- [ ] **Step 1: PacketGenerator 실행**

```powershell
cd E:\Projects\DH1_MMORPG
& Shared\BuildScripts\PacketGenerator.bat
```

Expected: `[Success] All proto files compiled successfully.` 출력 후 정상 종료

- [ ] **Step 2: 생성 결과 diff 비교**

```powershell
$dirs = @(
    "DH1_Engine\EchoClient\PacketHandler",
    "DH1_Engine\EchoServer\PacketHandler",
    "DH1_Client\Source\DH1_Client\Network\PacketHandler",
    "DH1_Server\GatewayServer\PacketHandler",
    "DH1_Server\WorldServer\PacketHandler",
    "DH1_Server\RealmServer\PacketHandler",
    "Shared\Protocol\PacketId"
)
foreach ($d in $dirs) {
    $snap = ".snapshot\$($d -replace '\\','_')"
    git diff --no-index -- $snap $d
}
```

Expected: `MakeSendBuffer` 호출부에서 `packet_id::MessageName` → `packet_id::eXxxPacketId::MessageName` 변경만 나타남. 그 외 차이가 있으면 회귀이므로 원인 파악 필요.

- [ ] **Step 3: 스냅샷 폴더 정리**

```powershell
Remove-Item -Recurse -Force .snapshot
```

`.gitignore`의 `.snapshot/` 항목은 그대로 유지한다 (향후 재사용 방지).

---

## Task 7: 최종 정리

- [ ] **Step 1: 전체 빌드 최종 확인**

```powershell
dotnet build Shared/Tools/PacketGenerator/PacketGenerator.csproj -c Release
```

- [ ] **Step 2: 변경 생성 파일 커밋**

PacketGenerator 실행으로 변경된 생성 파일(MakeSendBuffer enum 스코프 변경)을 커밋한다. 스냅샷과 비교했던 7개 디렉토리만 정확히 stage한다:

```powershell
git add DH1_Engine/EchoClient/PacketHandler/ DH1_Engine/EchoServer/PacketHandler/ DH1_Client/Source/DH1_Client/Network/PacketHandler/ DH1_Server/GatewayServer/PacketHandler/ DH1_Server/WorldServer/PacketHandler/ DH1_Server/RealmServer/PacketHandler/ Shared/Protocol/PacketId/
git commit -m "chore: regenerate packet handler headers with consistent enum scope"
```

**Note:** M2 (CodedInputStream 파싱 패턴 중복)는 스펙의 비범위(Out of Scope)에 따라 이번 정리에서 제외됨.
