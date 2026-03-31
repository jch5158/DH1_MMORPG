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
set "CLIENT_ARGS=-game -windowed"
set "LOGIN_HTTP_HOST=127.0.0.1"
set "LOGIN_HTTP_PORT=5000"
set "LOGIN_HEALTH_PATH=/health"

:: AWS CLI 경로 보정 (WorldServer의 `aws s3 cp` 실행 보장)
if exist "C:\Program Files\Amazon\AWSCLIV2\aws.exe" (
    set "PATH=C:\Program Files\Amazon\AWSCLIV2;%PATH%"
    set "DH1_AWS_CLI_PATH=C:\Program Files\Amazon\AWSCLIV2\aws.exe"
    echo [Info] AWS CLI path added for server processes
    echo [Info] DH1_AWS_CLI_PATH configured
)

:: 선택 해상도: .env에서 DH1_CLIENT_RESX / DH1_CLIENT_RESY 둘 다 설정된 경우에만 적용
if not "%DH1_CLIENT_RESX%"=="" if not "%DH1_CLIENT_RESY%"=="" (
    set "CLIENT_ARGS=%CLIENT_ARGS% -ResX=%DH1_CLIENT_RESX% -ResY=%DH1_CLIENT_RESY%"
    echo [Info] Client resolution override: %DH1_CLIENT_RESX%x%DH1_CLIENT_RESY%
)

:: 선택 포트: .env에서 DH1_LOGIN_HTTP_PORT 설정 시 LoginServer readiness 체크 포트 override
if not "%DH1_LOGIN_HTTP_PORT%"=="" (
    set "LOGIN_HTTP_PORT=%DH1_LOGIN_HTTP_PORT%"
    echo [Info] LoginServer readiness port override: %LOGIN_HTTP_PORT%
)
if not "%DH1_LOGIN_HEALTH_PATH%"=="" (
    set "LOGIN_HEALTH_PATH=%DH1_LOGIN_HEALTH_PATH%"
    echo [Info] LoginServer health path override: %LOGIN_HEALTH_PATH%
)
echo [Info] LoginServer readiness target: %LOGIN_HTTP_HOST%:%LOGIN_HTTP_PORT%

if not exist "%SERVER_DIR%\RealmServer\RealmServer.exe" (
    echo [Error] RealmServer.exe not found at %SERVER_DIR%\RealmServer\
    echo Please run BuildAll.bat first.
    pause
    exit /b 1
)

echo.
echo [1/5] Starting RealmServer...
start "RealmServer" "%SERVER_DIR%\RealmServer\RealmServer.exe"
call :wait_process "RealmServer.exe" 10
if errorlevel 1 goto :launch_error

echo [2/5] Starting WorldServer...
start "WorldServer" "%SERVER_DIR%\WorldServer\WorldServer.exe"
call :wait_process "WorldServer.exe" 10
if errorlevel 1 goto :launch_error

echo [3/5] Starting GatewayServer...
start "GatewayServer" "%SERVER_DIR%\GatewayServer\GatewayServer.exe"
call :wait_process "GatewayServer.exe" 10
if errorlevel 1 goto :launch_error

echo [4/5] Starting LoginServer...
start "LoginServer" dotnet run --no-launch-profile --project "%BASE_DIR%\DH1_Server\LoginServer\LoginServer.csproj"
call :wait_process "dotnet.exe" 10
if errorlevel 1 goto :launch_error
call :wait_tcp_port "%LOGIN_HTTP_HOST%" "%LOGIN_HTTP_PORT%" 20
if errorlevel 1 goto :launch_error
call :wait_http_ok "%LOGIN_HTTP_HOST%" "%LOGIN_HTTP_PORT%" "%LOGIN_HEALTH_PATH%" 20
if errorlevel 1 goto :launch_error

echo [5/5] Starting DH1_Client (UnrealEditor -game)...
if exist "%UE5_EDITOR%" (
    start "DH1_Client" "%UE5_EDITOR%" "%UPROJECT%" %CLIENT_ARGS%
    call :wait_process "UnrealEditor.exe" 20
) else (
    echo [Warning] UnrealEditor.exe not found: %UE5_EDITOR%
    echo           Install Unreal Engine 5.7 via Epic Games Launcher.
)

echo.
echo ============================================
echo  All processes started
echo  (No auto-stop in redirected terminal mode)
echo ============================================
echo Use StopGame.bat or Task Manager to stop processes.
exit /b 0

:launch_error
echo [Error] Process launch check failed.
echo [Error] See each server window/log for details.
exit /b 1

:wait_process
set "WAIT_IMAGE=%~1"
set "WAIT_TIMEOUT=%~2"
if "%WAIT_TIMEOUT%"=="" set "WAIT_TIMEOUT=10"
set /a "WAIT_REMAIN=%WAIT_TIMEOUT%" >nul 2>&1

:wait_process_loop
tasklist /FI "IMAGENAME eq %WAIT_IMAGE%" 2>nul | find /I "%WAIT_IMAGE%" >nul
if not errorlevel 1 exit /b 0
if %WAIT_REMAIN% LEQ 0 (
    echo [Error] %WAIT_IMAGE% did not appear within %WAIT_TIMEOUT%s.
    exit /b 1
)
set /a "WAIT_REMAIN-=1" >nul 2>&1
call :sleep_seconds 1
goto :wait_process_loop

:wait_tcp_port
set "TCP_HOST=%~1"
set "TCP_PORT=%~2"
set "TCP_TIMEOUT=%~3"
if "%TCP_HOST%"=="" set "TCP_HOST=127.0.0.1"
if "%TCP_PORT%"=="" set "TCP_PORT=5280"
if "%TCP_TIMEOUT%"=="" set "TCP_TIMEOUT=20"
set /a "TCP_REMAIN=%TCP_TIMEOUT%" >nul 2>&1

:wait_tcp_port_loop
powershell -NoProfile -Command "$client = New-Object System.Net.Sockets.TcpClient; try { $iar = $client.BeginConnect('%TCP_HOST%', [int]%TCP_PORT%, $null, $null); if ($iar.AsyncWaitHandle.WaitOne(800)) { $client.EndConnect($iar); exit 0 } else { exit 1 } } catch { exit 1 } finally { $client.Close() }" >nul 2>&1
if not errorlevel 1 (
    echo [Info] LoginServer TCP ready: %TCP_HOST%:%TCP_PORT%
    exit /b 0
)
if %TCP_REMAIN% LEQ 0 (
    echo [Error] LoginServer TCP not ready: %TCP_HOST%:%TCP_PORT% within %TCP_TIMEOUT%s.
    exit /b 1
)
set /a "TCP_REMAIN-=1" >nul 2>&1
call :sleep_seconds 1
goto :wait_tcp_port_loop

:wait_http_ok
set "HTTP_HOST=%~1"
set "HTTP_PORT=%~2"
set "HTTP_PATH=%~3"
set "HTTP_TIMEOUT=%~4"
if "%HTTP_HOST%"=="" set "HTTP_HOST=127.0.0.1"
if "%HTTP_PORT%"=="" set "HTTP_PORT=5000"
if "%HTTP_PATH%"=="" set "HTTP_PATH=/health"
if "%HTTP_TIMEOUT%"=="" set "HTTP_TIMEOUT=20"
set /a "HTTP_REMAIN=%HTTP_TIMEOUT%" >nul 2>&1

:wait_http_ok_loop
powershell -NoProfile -Command "$r = Invoke-WebRequest -Uri ('http://%HTTP_HOST%:%HTTP_PORT%%HTTP_PATH%') -UseBasicParsing -TimeoutSec 2; if ($r.StatusCode -eq 200) { exit 0 } else { exit 1 }" >nul 2>&1
if not errorlevel 1 (
    echo [Info] LoginServer health ready: http://%HTTP_HOST%:%HTTP_PORT%%HTTP_PATH%
    exit /b 0
)
if %HTTP_REMAIN% LEQ 0 (
    echo [Error] LoginServer health check failed: http://%HTTP_HOST%:%HTTP_PORT%%HTTP_PATH% within %HTTP_TIMEOUT%s.
    exit /b 1
)
set /a "HTTP_REMAIN-=1" >nul 2>&1
call :sleep_seconds 1
goto :wait_http_ok_loop

:sleep_seconds
set "SLEEP_SECONDS=%~1"
if "%SLEEP_SECONDS%"=="" set "SLEEP_SECONDS=1"
timeout /t %SLEEP_SECONDS% /nobreak >nul 2>&1
if not errorlevel 1 exit /b 0
powershell -NoProfile -Command "Start-Sleep -Seconds %SLEEP_SECONDS%" >nul 2>&1
if not errorlevel 1 exit /b 0
set /a "PING_COUNT=SLEEP_SECONDS+1" >nul 2>&1
ping 127.0.0.1 -n %PING_COUNT% >nul 2>&1
exit /b 0
