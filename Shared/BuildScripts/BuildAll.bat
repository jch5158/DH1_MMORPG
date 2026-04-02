@echo off
chcp 65001 >nul 2>&1
setlocal enabledelayedexpansion

:: BuildAll.bat - One-click full build
::
:: Usage:
::   BuildAll.bat                    Debug x64 full build
::   BuildAll.bat Release            Release x64 full build
::   BuildAll.bat --skip-proto       Skip proto generation
::   BuildAll.bat Release --skip-proto

set "CONFIG=Debug"
set "PLATFORM=x64"
set "SKIP_PROTO=0"

for %%A in (%*) do (
    if /i "%%A"=="--skip-proto" set "SKIP_PROTO=1"
    if /i "%%A"=="Release" set "CONFIG=Release"
    if /i "%%A"=="Debug" set "CONFIG=Debug"
)

set "BASE_DIR=%~dp0"
set "ROOT_DIR=%BASE_DIR%..\.."
set "START_TIME=%TIME%"

echo.
echo ============================================================
echo  DH1_MMORPG Build All - %CONFIG% ^| %PLATFORM%
echo ============================================================
echo.

:: ── 환경 검증 ─────────────────────────────────────────────────

call "%BASE_DIR%_common.bat" :check_dotnet_sdk
if errorlevel 1 exit /b 1

:: Visual Studio 버전 확인 (최소 18.0 = VS 2025)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%v in (`"!VSWHERE!" -latest -property installationVersion 2^>nul`) do set "VS_VER=%%v"
for /f "tokens=1 delims=." %%m in ("!VS_VER!") do set "VS_MAJOR=%%m"
if !VS_MAJOR! lss 18 (
    echo [Error] Visual Studio 2025 ^(v18+^) required.
    echo         Installed: !VS_VER!
    echo         Open .vsconfig in Visual Studio Installer to install required components.
    exit /b 1
)

:: UE5 경로 감지
call "%BASE_DIR%_common.bat" :detect_ue5
if not exist "!UE5_ROOT!\Engine\Build\BatchFiles\Build.bat" (
    echo [Error] Unreal Engine 5.7 not found at: !UE5_ROOT!
    echo         Install UE 5.7 via Epic Games Launcher.
    exit /b 1
)

echo [Check] .NET SDK : !DOTNET_VER! OK
echo [Check] VS       : !VS_VER! OK
echo [Check] UE5      : 5.7 OK
echo.

:: ──────────────────────────────────────────────────────────────

call "%BASE_DIR%_common.bat" :load_env_file "%ROOT_DIR%\.env" ".env"
call "%BASE_DIR%_common.bat" :load_env_file "%ROOT_DIR%\.env.local" ".env.local"
echo.

:: Find MSBuild
call "%BASE_DIR%_common.bat" :detect_msbuild
if errorlevel 1 exit /b 1
echo [Info] MSBuild: %MSBUILD%
echo.

:: Step 1: PacketGenerator
if "%SKIP_PROTO%"=="0" (
    echo [1/5] Running PacketGenerator...
    dotnet build "%ROOT_DIR%\Shared\Tools\PacketGenerator\PacketGenerator.csproj" -c Release --verbosity quiet
    if !errorlevel! neq 0 (
        echo [FAILED] PacketGenerator.csproj build failed
        exit /b 1
    )
    call "%BASE_DIR%PacketGenerator.bat"
    if !errorlevel! neq 0 (
        echo [FAILED] PacketGenerator failed
        exit /b 1
    )
    echo [1/5] PacketGenerator done
    echo.
) else (
    echo [1/5] PacketGenerator skipped [--skip-proto]
    echo.
)

:: Step 2: DH1_Engine
echo [2/5] Building DH1_Engine...
"%MSBUILD%" "%ROOT_DIR%\DH1_Engine\DH1_Engine.slnx" -p:Configuration=%CONFIG% -p:Platform=%PLATFORM% -m -nologo -v:minimal
if !errorlevel! neq 0 (
    echo [FAILED] DH1_Engine build failed
    exit /b 1
)
echo [2/5] DH1_Engine done
echo.

:: Step 3: DH1_Server (restore first to avoid NU1105)
echo [3/5] Building DH1_Server...
dotnet restore "%ROOT_DIR%\DH1_Server\DH1_Server.slnx" --verbosity quiet >nul 2>&1
"%MSBUILD%" "%ROOT_DIR%\DH1_Server\DH1_Server.slnx" -p:Configuration=%CONFIG% -p:Platform=%PLATFORM% -m -nologo -v:minimal
if !errorlevel! neq 0 (
    echo [FAILED] DH1_Server build failed
    exit /b 1
)
echo [3/5] DH1_Server done
echo.

:: Step 4: UE5 Client (Game + Editor targets)
set "UBT_DLL=!UE5_ROOT!\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
if not exist "%UBT_DLL%" (
    echo [Error] UnrealBuildTool.dll not found: %UBT_DLL%
    exit /b 1
)

echo [4/5] Building DH1_Client (UE5 Game target)...
dotnet "%UBT_DLL%" DH1_Client Win64 Development "-Project=%ROOT_DIR%\DH1_Client\DH1_Client.uproject" -NoUBA
if !errorlevel! neq 0 (
    echo [FAILED] UE5 Game target build failed
    exit /b 1
)
echo [4/5] DH1_Client done
echo.

echo [5/5] Building DH1_ClientEditor (UE5 Editor target)...
dotnet "%UBT_DLL%" DH1_ClientEditor Win64 Development "-Project=%ROOT_DIR%\DH1_Client\DH1_Client.uproject" -NoUBA
if !errorlevel! neq 0 (
    echo [FAILED] UE5 Editor target build failed
    exit /b 1
)
echo [5/5] DH1_ClientEditor done
echo.

:: Done
echo ============================================================
echo  Build complete (%CONFIG% ^| %PLATFORM%) [5/5]
echo  Start: %START_TIME%
echo  End:   %TIME%
echo ============================================================
echo.
echo [Info] Run StartServers.bat to launch servers + client.
echo.
exit /b 0
