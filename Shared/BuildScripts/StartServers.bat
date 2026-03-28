@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

set "BASE_DIR=%~dp0..\.."
set "CONFIG=%~1"

if "%CONFIG%"=="" (
    set "CONFIG=Debug"
)

echo ============================================
echo  DH1 MMORPG Server Launcher [%CONFIG%]
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

if not exist "%SERVER_DIR%\RealmServer\RealmServer.exe" (
    echo [Error] RealmServer.exe not found at %SERVER_DIR%\RealmServer\
    echo Please build the solution first.
    pause
    exit /b 1
)

echo.
echo [1/4] Starting RealmServer...
start "RealmServer" "%SERVER_DIR%\RealmServer\RealmServer.exe"
timeout /t 2 /nobreak >nul

echo [2/4] Starting WorldServer...
start "WorldServer" "%SERVER_DIR%\WorldServer\WorldServer.exe"
timeout /t 2 /nobreak >nul

echo [3/4] Starting GatewayServer...
start "GatewayServer" "%SERVER_DIR%\GatewayServer\GatewayServer.exe"
timeout /t 2 /nobreak >nul

echo [4/4] Starting LoginServer...
start "LoginServer" dotnet run --project "%BASE_DIR%\DH1_Server\LoginServer\LoginServer.csproj"
timeout /t 2 /nobreak >nul

echo.
echo ============================================
echo  All servers started!
echo  Press any key to stop all servers...
echo ============================================
pause >nul

echo.
echo Stopping servers...
taskkill /IM RealmServer.exe /F >nul 2>&1
taskkill /IM WorldServer.exe /F >nul 2>&1
taskkill /IM GatewayServer.exe /F >nul 2>&1
taskkill /IM LoginServer.exe /F >nul 2>&1
taskkill /IM dotnet.exe /F >nul 2>&1

echo All servers stopped.
