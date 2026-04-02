@echo off
rem _common.bat — Shared subroutines for DH1_MMORPG build scripts
rem
rem Usage from other scripts:
rem   call "%~dp0_common.bat" :subroutine_name arg1 arg2 ...
rem
rem Available subroutines:
rem   :load_env_file       <filepath> <label>
rem   :detect_ue5          (sets UE5_ROOT, UE5_EDITOR)
rem   :detect_aws_cli      (sets AWS_CMD, DH1_AWS_CLI_PATH, AWS_PROFILE)
rem   :check_aws_credentials
rem   :check_dotnet_sdk    (validates .NET SDK 10.0.201)
rem   :detect_msbuild      (sets MSBUILD via vswhere)
rem   :sleep_seconds       <seconds>

if "%~1"=="" exit /b 0
goto %~1

rem ── .env 파일 로드 ──────────────────────────────────────────
:load_env_file
set "ENV_FILE=%~2"
set "ENV_LABEL=%~3"
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

rem ── UE5 경로 감지 (레지스트리 → fallback) ──────────────────
:detect_ue5
if not "%DH1_UE5_EDITOR%"=="" (
    set "UE5_EDITOR=%DH1_UE5_EDITOR%"
    for %%F in ("!UE5_EDITOR!") do set "UE5_ROOT=%%~dpF..\.."
    exit /b 0
)
set "UE5_ROOT="
for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.7" /v InstalledDirectory 2^>nul') do set "UE5_ROOT=%%B"
if "!UE5_ROOT!"=="" set "UE5_ROOT=C:\Program Files\Epic Games\UE_5.7"
set "UE5_EDITOR=!UE5_ROOT!\Engine\Binaries\Win64\UnrealEditor.exe"
exit /b 0

rem ── AWS CLI 경로 + 프로필 자동 감지 ────────────────────────
:detect_aws_cli
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
if "!AWS_CMD!"=="aws" if exist "C:\Program Files\Amazon\AWSCLIV2\aws.exe" (
    set "PATH=C:\Program Files\Amazon\AWSCLIV2;%PATH%"
    set "DH1_AWS_CLI_PATH=C:\Program Files\Amazon\AWSCLIV2\aws.exe"
    set "AWS_CMD=!DH1_AWS_CLI_PATH!"
    echo [Info] AWS CLI path added for server processes
)
if not "%DH1_AWS_PROFILE%"=="" (
    set "AWS_PROFILE=%DH1_AWS_PROFILE%"
    echo [Info] AWS profile from .env: !AWS_PROFILE!
)
if "!AWS_PROFILE!"=="" (
    if exist "%USERPROFILE%\.aws\credentials" (
        findstr /B /C:"[dh1]" "%USERPROFILE%\.aws\credentials" >nul 2>&1
        if not errorlevel 1 (
            set "AWS_PROFILE=dh1"
            echo [Info] AWS profile auto-selected: dh1
        )
    )
)
exit /b 0

rem ── AWS 자격 증명 확인 ──────────────────────────────────────
:check_aws_credentials
if "!AWS_CMD!"=="aws" (
    where aws >nul 2>&1
    if errorlevel 1 (
        echo [Error] AWS CLI not found in PATH.
        echo [Error] Install AWS CLI v2 or set DH1_AWS_CLI_PATH in .env
        exit /b 1
    )
) else (
    if not exist "!AWS_CMD!" (
        echo [Error] AWS CLI path is invalid: !AWS_CMD!
        echo [Error] Update DH1_AWS_CLI_PATH in .env
        exit /b 1
    )
)
if "!AWS_PROFILE!"=="" (
    "!AWS_CMD!" sts get-caller-identity --output json >nul 2>&1
) else (
    "!AWS_CMD!" sts get-caller-identity --output json --profile "!AWS_PROFILE!" >nul 2>&1
)
if errorlevel 1 (
    echo [Error] AWS credentials are not available for this terminal/session.
    if not "!AWS_PROFILE!"=="" echo [Error] Current AWS_PROFILE: !AWS_PROFILE!
    exit /b 1
)
echo [Info] AWS credentials check passed.
exit /b 0

rem ── .NET SDK 버전 확인 ──────────────────────────────────────
:check_dotnet_sdk
for /f "usebackq delims=" %%v in (`dotnet --version 2^>nul`) do set "DOTNET_VER=%%v"
if not defined DOTNET_VER (
    echo [Error] .NET SDK not found. Install .NET 10.0.201.
    exit /b 1
)
if not "!DOTNET_VER!"=="10.0.201" (
    echo [Error] .NET SDK version mismatch.
    echo         Required : 10.0.201
    echo         Installed: !DOTNET_VER!
    echo         Download : https://dotnet.microsoft.com/download/dotnet/10.0
    exit /b 1
)
exit /b 0

rem ── MSBuild 경로 감지 (vswhere) ────────────────────────────
:detect_msbuild
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
exit /b 0

rem ── 대기 (timeout → PowerShell → ping fallback) ────────────
:sleep_seconds
set "_SLEEP_SEC=%~2"
if "%_SLEEP_SEC%"=="" set "_SLEEP_SEC=1"
timeout /t %_SLEEP_SEC% /nobreak >nul 2>&1
if not errorlevel 1 exit /b 0
powershell -NoProfile -Command "Start-Sleep -Seconds %_SLEEP_SEC%" >nul 2>&1
if not errorlevel 1 exit /b 0
set /a "_PING_N=_SLEEP_SEC+1" >nul 2>&1
ping 127.0.0.1 -n %_PING_N% >nul 2>&1
exit /b 0
