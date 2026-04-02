@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: StartClient.bat — DH1_Client 증분 빌드 후 UnrealEditor -game 으로 N개 실행
::
:: Usage:
::   StartClient.bat              → 빌드 + 1개 실행 (기본 해상도 1280x720)
::   StartClient.bat 3            → 빌드 + 3개 (기본 해상도 960x540, 창 위치 살짝 어긋남)
::   StartClient.bat 3 4          → 빌드 + 3개, 인스턴스 간 대기 4초 (기본 2초)
::   StartClient.bat --skip-build → 빌드 생략, 바로 실행
::
:: .env 선택 항목:
::   DH1_UE5_EDITOR           — UnrealEditor.exe 전체 경로 (미설정 시 레지스트리/기본 경로)
::   DH1_CLIENT_RESX / RESY   — 해상도
::   DH1_CLIENT_STAGGER_SEC   — 인스턴스 간 대기(초)
::   DH1_CLIENT_MULTI_COUNT   — 기본 실행 개수
::   DH1_CLIENT_SKIP_BUILD    — 1 이면 항상 빌드 생략

set "BASE_DIR=%~dp0..\.."
set "SKIP_BUILD=0"

REM 인자 파싱 (--skip-build 분리)
set "ARG1="
set "ARG2="
for %%A in (%*) do (
    if /i "%%A"=="--skip-build" (
        set "SKIP_BUILD=1"
    ) else if not defined ARG1 (
        set "ARG1=%%A"
    ) else if not defined ARG2 (
        set "ARG2=%%A"
    )
)

call :load_env_file "%BASE_DIR%\.env" ".env"
call :load_env_file "%BASE_DIR%\.env.local" ".env.local"

if not "%DH1_CLIENT_SKIP_BUILD%"=="" if "%DH1_CLIENT_SKIP_BUILD%"=="1" set "SKIP_BUILD=1"

set "INSTANCE_COUNT=%ARG1%"
set "STAGGER_SEC=%ARG2%"
if "%INSTANCE_COUNT%"=="" set "INSTANCE_COUNT=1"
if "%STAGGER_SEC%"=="" set "STAGGER_SEC=2"

if not "%DH1_CLIENT_STAGGER_SEC%"=="" set "STAGGER_SEC=%DH1_CLIENT_STAGGER_SEC%"
if "%ARG1%"=="" if not "%DH1_CLIENT_MULTI_COUNT%"=="" set "INSTANCE_COUNT=%DH1_CLIENT_MULTI_COUNT%"

set /a "IC=%INSTANCE_COUNT%" 2>nul
if errorlevel 1 set "IC=1"
if not defined IC set "IC=1"
if %IC% LSS 1 set "IC=1"
if %IC% GTR 12 (
    echo [Warning] instance count capped at 12 ^(was %INSTANCE_COUNT%^).
    set "IC=12"
)
set "INSTANCE_COUNT=%IC%"

REM UE5 엔진 경로 감지
set "UE5_ROOT="
if not "%DH1_UE5_EDITOR%"=="" (
    set "UE5_EDITOR=%DH1_UE5_EDITOR%"
) else (
    for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.7" /v InstalledDirectory 2^>nul') do (
        set "UE5_ROOT=%%B"
        set "UE5_EDITOR=%%B\Engine\Binaries\Win64\UnrealEditor.exe"
    )
)
if "!UE5_ROOT!"=="" set "UE5_ROOT=C:\Program Files\Epic Games\UE_5.7"
if "!UE5_EDITOR!"=="" set "UE5_EDITOR=!UE5_ROOT!\Engine\Binaries\Win64\UnrealEditor.exe"
set "UPROJECT=%BASE_DIR%\DH1_Client\DH1_Client.uproject"

if not exist "!UE5_EDITOR!" (
    echo [Error] UnrealEditor.exe not found:
    echo         !UE5_EDITOR!
    echo         Set DH1_UE5_EDITOR in .env or install UE 5.7.
    exit /b 1
)
if not exist "%UPROJECT%" (
    echo [Error] .uproject not found:
    echo         %UPROJECT%
    exit /b 1
)

set "RESX=1920"
set "RESY=1080"
if not "%DH1_CLIENT_RESX%"=="" set "RESX=%DH1_CLIENT_RESX%"
if not "%DH1_CLIENT_RESY%"=="" set "RESY=%DH1_CLIENT_RESY%"

echo ============================================
echo  DH1 Client Launcher  x%INSTANCE_COUNT%  (Editor -game^)
echo ============================================
echo [Info] UE: !UE5_EDITOR!
echo [Info] Project: %UPROJECT%
echo [Info] Resolution: %RESX%x%RESY%   stagger: %STAGGER_SEC%s

REM ── 증분 빌드 ──────────────────────────────────────────────
if "%SKIP_BUILD%"=="1" (
    echo [Info] Build skipped ^(--skip-build^)
    echo.
) else (
    set "UBT_DLL=!UE5_ROOT!\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
    if not exist "!UBT_DLL!" (
        echo [Error] UnrealBuildTool.dll not found: !UBT_DLL!
        echo         UE 5.7 installation may be incomplete.
        exit /b 1
    )

    echo.
    echo [Build] DH1_ClientEditor incremental build...
    dotnet "!UBT_DLL!" DH1_ClientEditor Win64 Development "-Project=%UPROJECT%" -NoUBA
    if !errorlevel! neq 0 (
        echo [FAILED] Client build failed. Fix errors and retry.
        exit /b 1
    )
    echo [Build] Done
    echo.
)

REM ── 클라이언트 실행 ─────────────────────────────────────────
for /L %%i in (1,1,%INSTANCE_COUNT%) do (
    set /a "OFFX=48 + (%%i-1)*56"
    set /a "OFFY=40 + (%%i-1)*40"
    set "ABSLOG=%TEMP%\DH1_Client_%%i.log"
    echo [Start] #%%i / %INSTANCE_COUNT%   -WinX=!OFFX! -WinY=!OFFY!   log: !ABSLOG!
    start "DH1_Client #%%i" "!UE5_EDITOR!" "%UPROJECT%" -game -windowed -ResX=%RESX% -ResY=%RESY% -WinX=!OFFX! -WinY=!OFFY! -ABSLOG="!ABSLOG!"
    if %%i LSS %INSTANCE_COUNT% call :sleep_seconds %STAGGER_SEC%
)

echo.
echo [Done] Per-instance log: %TEMP%\DH1_Client_1.log … #%INSTANCE_COUNT%
exit /b 0

:sleep_seconds
set "SLEEP_SECONDS=%~1"
if "%SLEEP_SECONDS%"=="" set "SLEEP_SECONDS=2"
timeout /t %SLEEP_SECONDS% /nobreak >nul 2>&1
if not errorlevel 1 exit /b 0
powershell -NoProfile -Command "Start-Sleep -Seconds %SLEEP_SECONDS%" >nul 2>&1
exit /b 0

:load_env_file
set "ENV_FILE=%~1"
set "ENV_LABEL=%~2"
if exist "%ENV_FILE%" (
    for /f "usebackq tokens=1,* delims==" %%A in ("%ENV_FILE%") do (
        set "LINE=%%A"
        if not "!LINE:~0,1!"=="#" if not "%%A"=="" (
            set "%%A=%%B"
        )
    )
    echo [Info] Loaded %ENV_LABEL%
) else (
    echo [Info] %ENV_LABEL% not found, skipping
)
exit /b 0
