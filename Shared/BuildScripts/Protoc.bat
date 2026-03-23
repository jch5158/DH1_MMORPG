@echo off

set "BASE_DIR=%~dp0"
set "VCPKG_DIR=%BASE_DIR%..\vcpkg\vcpkg_installed"

set "PROTOC_PATH=%VCPKG_DIR%\x64-windows-static-md\tools\protobuf\protoc.exe"
set "GOOGLE_INC=%VCPKG_DIR%\x64-windows-static-md\include"
set "PROTO_DIR=%BASE_DIR%..\Protocol\Proto"

set "CPP_OUT_DIR=%BASE_DIR%..\..\Shared\Protocol"
set "CLIENT_CPP_OUT_DIR=%BASE_DIR%..\..\DH1_Client\Source\DH1_Client\Network\Protocol"

if not exist "%PROTOC_PATH%" (
    echo [Error] protoc.exe not found: %PROTOC_PATH%
    echo Please ensure vcpkg protobuf is installed.
    exit /b 1
)

if not exist "%CPP_OUT_DIR%" mkdir "%CPP_OUT_DIR%"
if not exist "%CLIENT_CPP_OUT_DIR%" mkdir "%CLIENT_CPP_OUT_DIR%"

set "PROTOC_FAILED=0"

for %%f in ("%PROTO_DIR%\*.proto") do (
    echo Compiling %%~nxf...
    "%PROTOC_PATH%" -I="%PROTO_DIR%" -I="%GOOGLE_INC%" --cpp_out="%CPP_OUT_DIR%" --cpp_out="%CLIENT_CPP_OUT_DIR%" --descriptor_set_out="%PROTO_DIR%\%%~nf.desc" --include_imports "%%f"
    if errorlevel 1 (
        echo [Error] Failed to compile %%~nxf
        set "PROTOC_FAILED=1"
    )
)

if "%PROTOC_FAILED%"=="1" (
    echo [Error] One or more proto files failed to compile.
    exit /b 1
)

echo [Success] All proto files compiled successfully.
