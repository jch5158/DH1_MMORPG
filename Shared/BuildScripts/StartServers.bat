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

call "%~dp0_common.bat" :load_env_file "%BASE_DIR%\.env" ".env"
call "%~dp0_common.bat" :load_env_file "%BASE_DIR%\.env.local" ".env.local"

call "%~dp0_common.bat" :detect_aws_cli
call "%~dp0_common.bat" :check_aws_credentials
if errorlevel 1 (
    echo [Error] WorldServer loads NavMesh from S3; AWS credentials are required.
    echo [Error] Set DH1_AWS_PROFILE or default credentials, or use StartGame.bat checks as reference.
    pause
    exit /b 1
)

set "SERVER_DIR=%BASE_DIR%\Binaries\Server\%CONFIG%"

if not exist "%SERVER_DIR%\RealmServer\RealmServer.exe" (
    echo [Error] RealmServer.exe not found at %SERVER_DIR%\RealmServer\
    echo Please run BuildAll.bat first.
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
start "LoginServer" dotnet run --no-launch-profile --project "%BASE_DIR%\DH1_Server\LoginServer\LoginServer.csproj"
timeout /t 3 /nobreak >nul

echo.
echo ============================================
echo  All servers started!
echo  Press any key to stop all servers...
echo ============================================
pause >nul

echo.
echo Stopping...
taskkill /IM RealmServer.exe /F >nul 2>&1
taskkill /IM WorldServer.exe /F >nul 2>&1
taskkill /IM GatewayServer.exe /F >nul 2>&1
taskkill /IM dotnet.exe /F >nul 2>&1

echo All servers stopped.
exit /b 0
