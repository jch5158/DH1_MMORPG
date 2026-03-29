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

| 변수 | 설명 |
|------|------|
| `DH1_MYSQL_HOST` | MySQL 호스트 (기본: 127.0.0.1) |
| `DH1_MYSQL_PORT` | MySQL 포트 (기본: 3306) |
| `DH1_MYSQL_USER` | MySQL 사용자 |
| `DH1_MYSQL_PASSWORD` | MySQL 비밀번호 |
| `DH1_REDIS_HOST` | Redis 호스트 (기본: 127.0.0.1) |
| `DH1_REDIS_PORT` | Redis 포트 (기본: 6379) |
| `DH1_NAVMESH_PATH` | NavMesh 바이너리 절대 경로 |
| `DH1_SMTP_HOST` | SMTP 서버 주소 (예: smtp.gmail.com) |
| `DH1_SMTP_SENDER_EMAIL` | 발신자 이메일 주소 |
| `DH1_SMTP_APP_PASSWORD` | SMTP 앱 비밀번호 (Gmail 기준: 앱 비밀번호) |

## 빌드

**전체 빌드** (Engine → Server → ProtoBridge)
```batch
Shared\BuildScripts\BuildAll.bat           # Debug
Shared\BuildScripts\BuildAll.bat Release   # Release
```

**패킷 코드 생성** (`.proto` 수정 후 실행)
```batch
Shared\BuildScripts\PacketGenerator.bat
```

> VS Code에서는 `Ctrl+Shift+B` → 빌드 태스크 선택

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
