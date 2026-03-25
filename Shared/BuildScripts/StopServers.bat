@echo off
echo Stopping all DH1 servers...

taskkill /IM RealmServer.exe /F >nul 2>&1
taskkill /IM WorldServer.exe /F >nul 2>&1
taskkill /IM GatewayServer.exe /F >nul 2>&1
taskkill /IM LoginServer.exe /F >nul 2>&1
taskkill /IM dotnet.exe /F >nul 2>&1

echo All servers stopped.
