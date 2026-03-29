# AGENTS.md

## Cursor Cloud specific instructions

### Platform notes

This is primarily a **Windows C++ IOCP MMORPG project**. The C++ servers (RealmServer, WorldServer, GatewayServer, EchoServer/EchoClient) and the networking engine (CppNetEngine) **cannot be built or run on Linux** — they require Visual Studio 2025 with MSBuild and Windows IOCP APIs. On Linux (Cursor Cloud), only the C#/.NET components are buildable and testable.

### What works on Linux (Cursor Cloud)

| Component | Path | What to do |
|-----------|------|------------|
| **LoginServer** (ASP.NET Core 10) | `DH1_Server/LoginServer/` | `dotnet build`, `dotnet run` (ports 5000/5001) |
| **LoginServer.Tests** (xUnit) | `DH1_Server/LoginServer.Tests/` | `dotnet test` (51 tests, all in-memory, no external deps) |
| **PacketGenerator** (C# .NET 10) | `Shared/Tools/PacketGenerator/` | `dotnet build` (generates packet code from .proto files) |
| **DbMigration** (EF Core) | `DH1_Server/DbMigration/` | `dotnet ef database update --context AccountDbContext` / `--context GameDbContext` |

### .NET SDK

The project pins **.NET SDK 10.0.201** via `global.json` with `rollForward: disable`. The install script in the update script handles this automatically.

### Running LoginServer

LoginServer requires **MySQL** and **Redis** to start:

```bash
# Start services (if not already running)
redis-server --daemonize yes
sudo mysqld --user=mysql --daemonize

# Set env vars (defaults work for local dev with no password)
export DH1_MYSQL_PASSWORD=""
export DH1_REDIS_HOST=127.0.0.1
export DH1_REDIS_PORT=6379

# Run migrations (first time or after schema changes)
dotnet ef database update --project DH1_Server/DbMigration/DbMigration.csproj --context AccountDbContext
dotnet ef database update --project DH1_Server/DbMigration/DbMigration.csproj --context GameDbContext

# Start the server
dotnet run --project DH1_Server/LoginServer/LoginServer.csproj
```

The server uses HTTPS redirect; test with `curl -k` for self-signed certs, e.g.:
```bash
curl -s -k -X POST https://localhost:5001/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"user@example.com","password":"Password123!"}'
```

### Running tests

```bash
dotnet test DH1_Server/LoginServer.Tests/LoginServer.Tests.csproj
```

All 51 tests use in-memory DB mocks and do not require MySQL/Redis.

### Linting / code quality

C# projects use `<TreatWarningsAsErrors>true</TreatWarningsAsErrors>` (set in `Directory.Build.props`). A zero-warning build is enforced. Run `dotnet build` to check for warnings-as-errors.

### Environment variables

Copy `.env.example` to `.env` for reference. Key variables: `DH1_MYSQL_HOST`, `DH1_MYSQL_PORT`, `DH1_MYSQL_PASSWORD`, `DH1_REDIS_HOST`, `DH1_REDIS_PORT`. SMTP vars are optional (email verification degrades gracefully without them).

### EF Core migrations tool

`dotnet-ef` must be installed as a global tool: `dotnet tool install --global dotnet-ef`. The update script handles this. Make sure `~/.dotnet/tools` is on PATH.
