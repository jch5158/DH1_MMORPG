@echo off
set "BASE_DIR=%~dp0"

set "PROTOC_DIR=%BASE_DIR%..\vcpkg_installed\x64-windows-static-md\tools\protobuf"
set "PROTOC_PATH=%PROTOC_DIR%\protoc.exe"
set "GOOGLE_INC=%BASE_DIR%..\vcpkg_installed\x64-windows-static-md\include"
set "PROTO_DIR=%BASE_DIR%..\Protocol"

set "CPP_OUT_ENGINE_DIR=%BASE_DIR%..\..\DH1_Engine\Shared\Protocol"
if not exist "%CPP_OUT_ENGINE_DIR%" mkdir "%CPP_OUT_ENGINE_DIR%"

set "CPP_OUT_SERVER_DIR=%BASE_DIR%..\..\DH1_Server\Shared\Protocol"
if not exist "%CPP_OUT_SERVER_DIR%" mkdir "%CPP_OUT_SERVER_DIR%"

set "CPP_OUT_CLIENT_DIR=%BASE_DIR%..\..\DH1_Client\Source\DH1_Client\Network\Protocol"
if not exist "%CPP_OUT_CLIENT_DIR%" mkdir "%CPP_OUT_CLIENT_DIR%"

for %%f in ("%PROTO_DIR%\*.proto") do (
    "%PROTOC_PATH%" -I="%PROTO_DIR%" -I="%GOOGLE_INC%" --cpp_out="%CPP_OUT_ENGINE_DIR%" --cpp_out="%CPP_OUT_SERVER_DIR%" --cpp_out="%CPP_OUT_CLIENT_DIR%" --descriptor_set_out="%PROTO_DIR%\%%~nf.desc" --include_imports "%%f"
)