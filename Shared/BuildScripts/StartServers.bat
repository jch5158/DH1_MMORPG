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

:: 환경 변수 로드 (local > env > default)
call :load_env_file "%BASE_DIR%\.env" ".env"
call :load_env_file "%BASE_DIR%\.env.local" ".env.local"

set "AWS_CMD=aws"
if not "%DH1_AWS_CLI_PATH%"=="" (
    if exist "%DH1_AWS_CLI_PATH%" (
        for %%D in ("%DH1_AWS_CLI_PATH%") do set "PATH=%%~dpD;%PATH%"
        set "AWS_CMD=%DH1_AWS_CLI_PATH%"
        echo [Info] Using AWS CLI from DH1_AWS_CLI_PATH
    ) else (
        echo [Warning] DH1_AWS_CLI_PATH does not exist: %DH1_AWS_CLI_PATH%
    )
)
if "%AWS_CMD%"=="aws" if exist "C:\Program Files\Amazon\AWSCLIV2\aws.exe" (
    set "PATH=C:\Program Files\Amazon\AWSCLIV2;%PATH%"
    set "DH1_AWS_CLI_PATH=C:\Program Files\Amazon\AWSCLIV2\aws.exe"
    set "AWS_CMD=!DH1_AWS_CLI_PATH!"
    echo [Info] AWS CLI path added for server processes
)
if not "%DH1_AWS_PROFILE%"=="" (
    set "AWS_PROFILE=%DH1_AWS_PROFILE%"
    echo [Info] AWS profile from .env: !AWS_PROFILE!
)
if "%AWS_PROFILE%"=="" (
    if exist "%USERPROFILE%\.aws\credentials" (
        findstr /B /C:"[dh1]" "%USERPROFILE%\.aws\credentials" >nul 2>&1
        if not errorlevel 1 (
            set "AWS_PROFILE=dh1"
            echo [Info] AWS profile auto-selected: dh1
        )
    )
)
call :check_aws_credentials
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
start "LoginServer" dotnet run --project "%BASE_DIR%\DH1_Server\LoginServer\LoginServer.csproj"
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

:check_aws_credentials
if "%AWS_CMD%"=="aws" (
    where aws >nul 2>&1
    if errorlevel 1 (
        echo [Error] AWS CLI not found in PATH.
        echo [Error] Install AWS CLI v2 or set DH1_AWS_CLI_PATH in .env
        exit /b 1
    )
) else (
    if not exist "%AWS_CMD%" (
        echo [Error] AWS CLI path is invalid: %AWS_CMD%
        echo [Error] Update DH1_AWS_CLI_PATH in .env
        exit /b 1
    )
)
if "%AWS_PROFILE%"=="" (
    "%AWS_CMD%" sts get-caller-identity --output json >nul 2>&1
) else (
    "%AWS_CMD%" sts get-caller-identity --output json --profile "%AWS_PROFILE%" >nul 2>&1
)
if errorlevel 1 (
    echo [Error] AWS credentials are not available for this terminal/session.
    echo [Error] Fix AWS login/profile first, then run StartServers.bat again.
    if not "%AWS_PROFILE%"=="" echo [Error] Current AWS_PROFILE: %AWS_PROFILE%
    exit /b 1
)
echo [Info] AWS credentials check passed.
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
