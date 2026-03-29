@echo off

set "BASE_DIR=%~dp0"
set "VCPKG_DIR=%BASE_DIR%..\vcpkg\vcpkg_installed"

set "PROTOC_PATH=%VCPKG_DIR%\x64-windows-static-md\tools\protobuf\protoc.exe"
set "GOOGLE_INC=%VCPKG_DIR%\x64-windows-static-md\include"
set "PROTO_DIR=%BASE_DIR%..\Protocol\Proto"
set "CPP_OUT_DIR=%BASE_DIR%..\..\Shared\Protocol"

if not exist "%PROTOC_PATH%" (
    echo [Error] protoc.exe not found: %PROTOC_PATH%
    echo Please ensure vcpkg protobuf is installed.
    pause
    exit /b 1
)

if not exist "%CPP_OUT_DIR%" mkdir "%CPP_OUT_DIR%"

set "PROTOC_FAILED=0"

for %%f in ("%PROTO_DIR%\*.proto") do (
    echo Compiling %%~nxf...
    "%PROTOC_PATH%" -I="%PROTO_DIR%" -I="%GOOGLE_INC%" --cpp_out="%CPP_OUT_DIR%" --descriptor_set_out="%PROTO_DIR%\%%~nf.desc" --include_imports "%%f"
    if errorlevel 1 (
        echo [Error] Failed to compile %%~nxf
        set "PROTOC_FAILED=1"
    )
)

if "%PROTOC_FAILED%"=="1" (
    echo [Error] One or more proto files failed to compile.
    pause
    exit /b 1
)

echo [Success] All proto files compiled successfully.

set "GENERATOR_EXE=%BASE_DIR%..\Tools\PacketGenerator\Binaries\PacketGenerator.exe"
set "ARG_CONFIG=%BASE_DIR%..\Config\Tools\PacketGeneratorConfig.json"
set "ARG_PROTO=%BASE_DIR%..\Protocol\Proto"
set "ARG_BASE_PRJ=%BASE_DIR%..\.."

if not exist "%GENERATOR_EXE%" (
    echo [Error] PacketGenerator is not exist
    echo Path: %GENERATOR_EXE%
    echo you changed the Enum.proto file, you must rebuild the PacketGenerator solution.
    pause
    exit /b 1
)

"%GENERATOR_EXE%" "%ARG_CONFIG%" "%ARG_PROTO%" "%ARG_BASE_PRJ%"

set "PACKET_ID_PATH=%BASE_DIR%..\Protocol\PacketId"
set "DH1_CLIENT_PATH=%ARG_BASE_PRJ%\DH1_Client\Source\DH1_Client\Network\Protocol\PacketId"

if not exist "%DH1_CLIENT_PATH%" mkdir "%DH1_CLIENT_PATH%"

xcopy /S /Y "%PACKET_ID_PATH%\PacketId.h" "%DH1_CLIENT_PATH%\"
