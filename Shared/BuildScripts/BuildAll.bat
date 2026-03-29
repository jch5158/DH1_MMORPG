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

:: .env 파일 로드 (루트 디렉토리)
if exist "%ROOT_DIR%\.env" (
    for /f "usebackq tokens=1,* delims==" %%A in ("%ROOT_DIR%\.env") do (
        set "LINE=%%A"
        if not "!LINE:~0,1!"=="#" if not "%%A"=="" (
            set "%%A=%%B"
        )
    )
    echo [Info] Loaded .env
) else (
    echo [Warning] .env not found, skipping
)
echo.

:: Find MSBuild via vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [Error] vswhere.exe not found
    exit /b 1
)

for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\amd64\MSBuild.exe`) do (
    set "MSBUILD=%%i"
)

if not defined MSBUILD (
    echo [Error] MSBuild.exe not found. Install Visual Studio.
    exit /b 1
)

echo [Info] MSBuild: %MSBUILD%
echo.

:: Step 1: PacketGenerator
if "%SKIP_PROTO%"=="0" (
    echo [1/4] Running PacketGenerator...
    call "%BASE_DIR%PacketGenerator.bat"
    if !errorlevel! neq 0 (
        echo [FAILED] PacketGenerator failed
        exit /b 1
    )
    echo [1/4] PacketGenerator done
    echo.
) else (
    echo [1/4] PacketGenerator skipped [--skip-proto]
    echo.
)

:: Step 2: DH1_Engine
echo [2/4] Building DH1_Engine...
"%MSBUILD%" "%ROOT_DIR%\DH1_Engine\DH1_Engine.slnx" -p:Configuration=%CONFIG% -p:Platform=%PLATFORM% -m -nologo -v:minimal
if !errorlevel! neq 0 (
    echo [FAILED] DH1_Engine build failed
    exit /b 1
)
echo [2/4] DH1_Engine done
echo.

:: Step 3: DH1_Server (restore first to avoid NU1105)
echo [3/4] Building DH1_Server...
dotnet restore "%ROOT_DIR%\DH1_Server\DH1_Server.slnx" --verbosity quiet >nul 2>&1
"%MSBUILD%" "%ROOT_DIR%\DH1_Server\DH1_Server.slnx" -p:Configuration=%CONFIG% -p:Platform=%PLATFORM% -m -nologo -v:minimal
if !errorlevel! neq 0 (
    echo [FAILED] DH1_Server build failed
    exit /b 1
)
echo [3/4] DH1_Server done
echo.

:: Step 4: ProtoBridge
echo [4/5] Building ProtoBridge...
"%MSBUILD%" "%ROOT_DIR%\DH1_Client\ProtoBridge\ProtoBridge.slnx" -p:Configuration=%CONFIG% -p:Platform=%PLATFORM% -m -nologo -v:minimal
if !errorlevel! neq 0 (
    echo [FAILED] ProtoBridge build failed
    exit /b 1
)
echo [4/5] ProtoBridge done
echo.

:: Step 5: UE5 Client (Development standalone)
echo [5/5] Building DH1_Client (UE5 Development)...
set "UBT_DLL=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
if not exist "%UBT_DLL%" (
    echo [Error] UnrealBuildTool.dll not found: %UBT_DLL%
    exit /b 1
)
dotnet "%UBT_DLL%" DH1_Client Win64 Development "-Project=%ROOT_DIR%\DH1_Client\DH1_Client.uproject"
if !errorlevel! neq 0 (
    echo [FAILED] UE5 client build failed
    exit /b 1
)
echo [5/5] DH1_Client done
echo.

:: Done
echo ============================================================
echo  Build complete (%CONFIG% ^| %PLATFORM%)
echo  Start: %START_TIME%
echo  End:   %TIME%
echo ============================================================
echo.
echo [Info] Run StartServers.bat to launch servers + client.
echo.
