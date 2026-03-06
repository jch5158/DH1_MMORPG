@echo off

:: 인자로 전달받은 매크로 경로들
set "SOLUTION_DIR=%~1"
set "PROJECT_DIR=%~2"
set "VCPKG_TRIPLET=%~3"

:: vcpkg 경로 설정
set "VCPKG_TARGET_DIR=%SOLUTION_DIR%vcpkg_installed\%VCPKG_TRIPLET%\%VCPKG_TRIPLET%"

set "PROTOC_PATH=%VCPKG_TARGET_DIR%\tools\protobuf"
set "VCPKG_INC=%VCPKG_TARGET_DIR%\include"
set "VCPKG_BIN=%VCPKG_TARGET_DIR%\bin"

:: protoc.exe 실행 시 필요한 종속성 DLL 임시 PATH 추가
set "PATH=%VCPKG_BIN%;%PATH%"

set "PROTO_DIR=%SOLUTION_DIR%Common\Protocol"
set "OUT_DIR=%PROJECT_DIR%Generated"

:: C++ 코드가 생성될 출력 폴더가 없으면 미리 생성
if not exist "%OUT_DIR%" (
    mkdir "%OUT_DIR%"
)

:: C++ 파일(.pb.cc, .pb.h)은 Generated 폴더로, desc 파일은 원본 프로토 폴더로 분리해서 출력
for %%f in ("%PROTO_DIR%\*.proto") do (
    "%PROTOC_PATH%\protoc.exe" -I="%PROTO_DIR%" -I="%VCPKG_INC%" --cpp_out="%OUT_DIR%" --descriptor_set_out="%PROTO_DIR%\%%~nf.desc" "%%f"
)