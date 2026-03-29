@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

set "BASE_DIR=%~dp0..\.."
set "CONFIG=%~1"

if "%CONFIG%"=="" (
    set "CONFIG=Debug"
)

echo ============================================
echo  DH1 MMORPG Game Launcher [%CONFIG%]
echo ============================================

:: .env 파일 로드 (루트 디렉토리)
if exist "%BASE_DIR%\.env" (
    for /f "usebackq tokens=1,* delims==" %%A in ("%BASE_DIR%\.env") do (
        set "LINE=%%A"
        if not "!LINE:~0,1!"=="#" if not "%%A"=="" (
            set "%%A=%%B"
        )
    )
    echo [Info] Loaded .env
) else (
    echo [Warning] .env not found, using defaults
)

set "SERVER_DIR=%BASE_DIR%\Binaries\Server\%CONFIG%"
set "UE5_EDITOR=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
set "UPROJECT=%BASE_DIR%\DH1_Client\DH1_Client.uproject"

if not exist "%SERVER_DIR%\RealmServer\RealmServer.exe" (
    echo [Error] RealmServer.exe not found at %SERVER_DIR%\RealmServer\
    echo Please run BuildAll.bat first.
    pause
    exit /b 1
)

echo.
echo [1/5] Starting RealmServer...
start "RealmServer" "%SERVER_DIR%\RealmServer\RealmServer.exe"
timeout /t 2 /nobreak >nul

echo [2/5] Starting WorldServer...
start "WorldServer" "%SERVER_DIR%\WorldServer\WorldServer.exe"
timeout /t 2 /nobreak >nul

echo [3/5] Starting GatewayServer...
start "GatewayServer" "%SERVER_DIR%\GatewayServer\GatewayServer.exe"
timeout /t 2 /nobreak >nul

echo [4/5] Starting LoginServer...
start "LoginServer" dotnet run --project "%BASE_DIR%\DH1_Server\LoginServer\LoginServer.csproj"
timeout /t 3 /nobreak >nul

echo [5/5] Starting DH1_Client (UnrealEditor -game)...
if exist "%UE5_EDITOR%" (
    start "DH1_Client" "%UE5_EDITOR%" "%UPROJECT%" -game -windowed -ResX=1280 -ResY=720
) else (
    echo [Warning] UnrealEditor.exe not found: %UE5_EDITOR%
    echo           Install Unreal Engine 5.7 via Epic Games Launcher.
)

echo.
echo ============================================
echo  All processes started!
echo  Press any key to stop everything...
echo ============================================
pause >nul

echo.
echo Stopping...
taskkill /IM RealmServer.exe /F >nul 2>&1
taskkill /IM WorldServer.exe /F >nul 2>&1
taskkill /IM GatewayServer.exe /F >nul 2>&1
taskkill /IM UnrealEditor.exe /F >nul 2>&1
taskkill /IM dotnet.exe /F >nul 2>&1

echo All processes stopped.
