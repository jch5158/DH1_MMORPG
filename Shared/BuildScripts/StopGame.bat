@echo off
chcp 65001 >nul
setlocal

set "SERVERS_ONLY=0"
if /i "%~1"=="--servers-only" set "SERVERS_ONLY=1"

if "%SERVERS_ONLY%"=="1" (
    echo ============================================
    echo  DH1 MMORPG Stop Servers
    echo ============================================
) else (
    echo ============================================
    echo  DH1 MMORPG Stop All
    echo ============================================
)

echo Stopping processes...

taskkill /IM RealmServer.exe /F >nul 2>&1
taskkill /IM WorldServer.exe /F >nul 2>&1
taskkill /IM GatewayServer.exe /F >nul 2>&1
taskkill /IM dotnet.exe /F >nul 2>&1

if "%SERVERS_ONLY%"=="0" (
    taskkill /IM UnrealEditor.exe /F >nul 2>&1
)

echo Done.
exit /b 0
