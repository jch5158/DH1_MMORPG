@echo off

set "BASE_DIR=%~dp0"

call "%BASE_DIR%Protoc.bat"

if %errorlevel% neq 0 (
    echo [Error] Protoc.bat execute failed
    pause
    exit /b %errorlevel%
)

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