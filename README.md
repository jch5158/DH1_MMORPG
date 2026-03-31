# DH1_MMORPG

C++ IOCP 서버 엔진 기반 MMORPG 프로젝트

## 구조

```
DH1_MMORPG/
├── DH1_Engine/          # CppNetEngine (IOCP 네트워크 엔진, static lib)
├── DH1_Server/
│   ├── GatewayServer/   # 클라이언트 접속 관문
│   ├── WorldServer/     # 게임 로직, NavMesh 경로탐색
│   ├── RealmServer/     # 서버 목록 / 세션 관리
│   └── LoginServer/     # 인증 (C# ASP.NET Core 10)
├── DH1_Client/          # UE5 클라이언트
└── Shared/
    ├── BuildScripts/    # 빌드 / 서버 실행 스크립트
    ├── Config/          # 서버/클라이언트 JSON 설정
    ├── NavMesh/         # 서버용 NavMesh 바이너리
    ├── NavmeshUE5/      # UE5 Detour 소스 (서버 컴파일용)
    ├── Protocol/        # Protobuf 생성 코드
    └── vcpkg/           # 패키지 의존성
```

## 환경 설정

```bash
# .env.example을 복사해 값을 채워주세요
cp .env.example .env
```

환경 설정은 `.env` 단일 파일 기준으로 운영합니다.

- `.env`: 로컬 실행용 환경 변수(커밋 금지, `.gitignore` 적용)
- `.env.example`: 템플릿(커밋 대상)

| 변수 | 설명 |
|------|------|
| `DH1_MYSQL_HOST` | MySQL 호스트 (기본: 127.0.0.1) |
| `DH1_MYSQL_PORT` | MySQL 포트 (기본: 3306) |
| `DH1_MYSQL_USER` | MySQL 사용자 |
| `DH1_MYSQL_PASSWORD` | MySQL 비밀번호 |
| `DH1_REDIS_HOST` | Redis 호스트 (기본: 127.0.0.1) |
| `DH1_REDIS_PORT` | Redis 포트 (기본: 6379) |
| `DH1_S3_REGION` | `s3://` 사용 시 AWS 리전 (`ap-northeast-2` 등) |
| `DH1_NAVMESH_CACHE_PATH` | S3에서 내려받은 NavMesh 로컬 캐시 경로 |
| `DH1_SMTP_HOST` | SMTP 서버 주소 (예: smtp.gmail.com) |
| `DH1_SMTP_SENDER_EMAIL` | 발신자 이메일 주소 |
| `DH1_SMTP_APP_PASSWORD` | SMTP 앱 비밀번호 (Gmail 기준: 앱 비밀번호) |
| `BRAVE_API_KEY` | Brave Search MCP API 키 |
| `GITHUB_PERSONAL_ACCESS_TOKEN` | GitHub MCP용 Personal Access Token |
| `FLOPPERAM_API_KEY` | Flopperam Unreal MCP API 키 |
| `FIRECRAWL_API_KEY` | Firecrawl MCP API 키 |

`Shared/Config/Server/WorldServerConfig.json`의 `gameTick.navMeshRequireSuccess`가 `true`이면(기본값)  
NavMesh 다운로드/로딩 실패 시 WorldServer는 fail-fast로 기동을 중단합니다.

### NavMesh 소스 DB (필수)

WorldServer는 `world_navmesh_source` 테이블이 있으면 `world_server_id + map_code` 기준으로 NavMesh 경로를 먼저 조회합니다.  
NavMesh 소스는 `s3://bucket/key`만 지원합니다.

```sql
CREATE TABLE world_navmesh_source (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  world_server_id INT NOT NULL,
  map_code VARCHAR(64) NOT NULL,
  navmesh_path VARCHAR(1024) NOT NULL,  -- s3://bucket/key
  navmesh_version INT NULL,
  is_active TINYINT(1) NOT NULL DEFAULT 1,
  UNIQUE KEY uq_world_map (world_server_id, map_code, is_active)
);
```

### 권장 운영 방식 (IAM)

- EC2(또는 서버)에 IAM Role 부여 (`s3:GetObject`)
- 서버에 AWS CLI 설치
- `world_navmesh_source.navmesh_path`를 `s3://...`로 관리
- WorldServer 시작 시 `aws s3 cp`로 캐시에 내려받아 로드

#### 최소 권한 정책 예시

- 파일: `Shared/Config/Server/IamPolicy.NavMeshReadOnly.example.json`
- 버킷명/접두어를 실제 운영 값으로 바꿔서 IAM Role에 연결

```bash
# 권한 연결 후 테스트 (서버 머신)
aws s3 ls s3://dh1-navmesh-prod/navmesh/ --region ap-northeast-2
aws s3 cp s3://dh1-navmesh-prod/navmesh/L_GameWorld.bin C:/Temp/L_GameWorld.bin --region ap-northeast-2
```

## 빌드

**전체 빌드** (PacketGenerator → Engine → Server → UE 클라이언트)
```batch
Shared\BuildScripts\BuildAll.bat           # Debug
Shared\BuildScripts\BuildAll.bat Release   # Release
```

**패킷 코드 생성** (`.proto` 수정 후 실행)
```batch
Shared\BuildScripts\PacketGenerator.bat
```

> VS Code에서는 `Ctrl+Shift+B` → 빌드 태스크 선택

**검증(테스트)** — CI와 동일한 최소 점검 (로컬, .NET SDK 10.0.201)
```batch
Shared\BuildScripts\RunValidationTests.bat
```
`--full` 옵션은 vcpkg `protoc`가 있을 때 proto 컴파일·패킷 생성·Echo 엔진 빌드까지 수행합니다.

## 서버 실행

```batch
Shared\BuildScripts\StartServers.bat          # 서버만 실행 (Debug)
Shared\BuildScripts\StartServers.bat Release  # 서버만 실행 (Release)

Shared\BuildScripts\StartGame.bat             # 서버 + 클라이언트 실행
```

실행 순서: RealmServer → WorldServer → GatewayServer → LoginServer → (클라이언트)

## 주요 의존성

| 항목 | 버전/내용 |
|------|----------|
| Visual Studio | 2025 (v18.x) |
| .NET SDK | 10.0.201 (`global.json`으로 버전 고정) |
| Unreal Engine | 5.7 |
| vcpkg 패키지 | spdlog, protobuf, redis-plus-plus, mimalloc 등 (`Shared/vcpkg/vcpkg.json`) |
