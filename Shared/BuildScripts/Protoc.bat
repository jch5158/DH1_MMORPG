@echo off
set "BASE_DIR=%~dp0"

set "PROTOC_DIR=%BASE_DIR%..\Tools\Protoc"
set "PROTOC_PATH=%PROTOC_DIR%\protoc.exe"
set "GOOGLE_INC=%PROTOC_DIR%\include"
set "PROTO_DIR=%BASE_DIR%..\Protocol"

set "CPP_OUT_DIR=%BASE_DIR%..\Protocol\Cpp"
if not exist "%CPP_OUT_DIR%" mkdir "%CPP_OUT_DIR%"

for %%f in ("%PROTO_DIR%\*.proto") do (
    "%PROTOC_PATH%" -I="%PROTO_DIR%" -I="%GOOGLE_INC%" --cpp_out="%CPP_OUT_DIR%" --descriptor_set_out="%PROTO_DIR%\%%~nf.desc" --include_imports "%%f"
)