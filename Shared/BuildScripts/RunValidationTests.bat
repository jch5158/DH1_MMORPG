@echo off
chcp 65001 >nul 2>&1
setlocal enabledelayedexpansion

:: RunValidationTests.bat — CI와 맞춘 최소 검증 (로컬 / 에이전트)
::
::   RunValidationTests.bat           PacketGenerator 빌드 + LoginServer 단위 테스트
::   RunValidationTests.bat --full    위 + protoc 패킷 생성 + DH1_Engine(Echo) MSBuild
::
:: 요구: .NET SDK 10.0.201 (global.json). --full 은 vcpkg protobuf(protoc) + VS MSBuild 필요.

set "BASE_DIR=%~dp0"
set "ROOT_DIR=%BASE_DIR%..\.."
set "FULL=0"

if /i "%~1"=="--full" set "FULL=1"

pushd "%ROOT_DIR%" || exit /b 1

call "%BASE_DIR%_common.bat" :check_dotnet_sdk
if errorlevel 1 ( popd & exit /b 1 )

echo [1/2] Building PacketGenerator ^(Release^)...
dotnet build "%ROOT_DIR%\Shared\Tools\PacketGenerator\PacketGenerator.csproj" -c Release --verbosity quiet
if !errorlevel! neq 0 (
    echo [FAILED] PacketGenerator build
    popd & exit /b 1
)

echo [2/2] dotnet test LoginServer.Tests...
dotnet test "%ROOT_DIR%\DH1_Server\LoginServer.Tests\LoginServer.Tests.csproj" -c Release --verbosity minimal
if !errorlevel! neq 0 (
    echo [FAILED] LoginServer.Tests
    popd & exit /b 1
)

if "!FULL!"=="0" (
    echo.
    echo [OK] Core validation passed. Use --full for proto compile + DH1_Engine build.
    popd & exit /b 0
)

echo.
echo [--full] Running PacketGenerator.bat ^(protoc + codegen^)...
call "%BASE_DIR%PacketGenerator.bat"
if !errorlevel! neq 0 (
    echo [FAILED] PacketGenerator.bat
    popd & exit /b 1
)

call "%BASE_DIR%_common.bat" :detect_msbuild
if errorlevel 1 ( popd & exit /b 1 )

echo [--full] Building DH1_Engine ^(Debug x64^)...
"%MSBUILD%" "%ROOT_DIR%\DH1_Engine\DH1_Engine.slnx" -p:Configuration=Debug -p:Platform=x64 -m -nologo -v:minimal
if !errorlevel! neq 0 (
    echo [FAILED] DH1_Engine
    popd & exit /b 1
)

echo.
echo [OK] Full validation passed.
popd & exit /b 0
