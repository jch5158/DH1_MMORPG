@echo off
chcp 65001 >nul
setlocal

echo ============================================
echo  DH1 MMORPG Stop Launcher
echo ============================================
echo Stopping processes...

taskkill /IM RealmServer.exe /F >nul 2>&1
taskkill /IM WorldServer.exe /F >nul 2>&1
taskkill /IM GatewayServer.exe /F >nul 2>&1
taskkill /IM UnrealEditor.exe /F >nul 2>&1
taskkill /IM dotnet.exe /F >nul 2>&1

echo Done.
exit /b 0
