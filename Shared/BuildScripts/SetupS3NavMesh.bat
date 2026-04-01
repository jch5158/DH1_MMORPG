@echo off
setlocal

set "BASE_DIR=%~dp0..\.."

REM Usage:
REM   SetupS3NavMesh.bat <BucketName> [Region] [NavMeshFilePath] [WorldServerId] [MapCode] [EnvPath]
REM Example:
REM   SetupS3NavMesh.bat dh1-navmesh-567017110299 ap-northeast-2

if "%~1"=="" (
    echo [Error] BucketName is required.
    echo Usage: %~nx0 ^<BucketName^> [Region] [NavMeshFilePath] [WorldServerId] [MapCode] [EnvPath]
    exit /b 1
)

set "BUCKET_NAME=%~1"
set "REGION=%~2"
set "NAVMESH_FILE=%~3"
set "WORLD_SERVER_ID=%~4"
set "MAP_CODE=%~5"
set "ENV_PATH=%~6"

if "%REGION%"=="" set "REGION=ap-northeast-2"
if "%NAVMESH_FILE%"=="" set "NAVMESH_FILE=%BASE_DIR%\Shared\NavMesh\L_GameWorld.bin"
if "%WORLD_SERVER_ID%"=="" set "WORLD_SERVER_ID=1"
if "%MAP_CODE%"=="" set "MAP_CODE=L_GameWorld"
if "%ENV_PATH%"=="" set "ENV_PATH=%BASE_DIR%\.env"

if not exist "%NAVMESH_FILE%" (
    echo [Error] NavMesh file not found: %NAVMESH_FILE%
    exit /b 1
)

if not exist "%ENV_PATH%" (
    echo [Error] .env file not found: %ENV_PATH%
    exit /b 1
)

set "AWS_EXE="
if exist "C:\Program Files\Amazon\AWSCLIV2\aws.exe" (
    set "AWS_EXE=C:\Program Files\Amazon\AWSCLIV2\aws.exe"
) else (
    for /f "usebackq delims=" %%i in (`where aws 2^>nul`) do (
        set "AWS_EXE=%%i"
        goto :aws_found
    )
)

:aws_found
if "%AWS_EXE%"=="" (
    echo [Error] AWS CLI not found. Install AWS CLI v2 first.
    exit /b 1
)

echo [Info] AWS CLI: %AWS_EXE%

"%AWS_EXE%" sts get-caller-identity >nul 2>nul
if errorlevel 1 (
    echo [Error] AWS credential check failed. Configure credentials first.
    exit /b 1
)
echo [Info] AWS credential check passed

"%AWS_EXE%" s3api head-bucket --bucket "%BUCKET_NAME%" >nul 2>nul
if errorlevel 1 (
    echo [Info] Creating bucket: %BUCKET_NAME% ^(%REGION%^)
    if /i "%REGION%"=="us-east-1" (
        "%AWS_EXE%" s3api create-bucket --bucket "%BUCKET_NAME%" >nul
    ) else (
        "%AWS_EXE%" s3api create-bucket --bucket "%BUCKET_NAME%" --create-bucket-configuration LocationConstraint=%REGION% >nul
    )
    if errorlevel 1 (
        echo [Error] Failed to create bucket.
        exit /b 1
    )
) else (
    echo [Info] Bucket already exists: %BUCKET_NAME%
)

"%AWS_EXE%" s3api put-public-access-block --bucket "%BUCKET_NAME%" --public-access-block-configuration BlockPublicAcls=true,IgnorePublicAcls=true,BlockPublicPolicy=true,RestrictPublicBuckets=true >nul
if errorlevel 1 (
    echo [Error] Failed to set PublicAccessBlock.
    exit /b 1
)

set "NAVMESH_FILE_WIN=%NAVMESH_FILE:/=\%"
for %%I in ("%NAVMESH_FILE_WIN%") do set "FILE_NAME=%%~nxI"
set "S3_KEY=navmesh/world_%WORLD_SERVER_ID%/%FILE_NAME%"
set "S3_URI=s3://%BUCKET_NAME%/%S3_KEY%"

echo [Info] Uploading NavMesh ^-^> %S3_URI%
"%AWS_EXE%" s3 cp "%NAVMESH_FILE%" "%S3_URI%" --region "%REGION%" --only-show-errors
if errorlevel 1 (
    echo [Error] Upload failed.
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$envPath = '%ENV_PATH%';" ^
  "$repoRoot = (Split-Path -Path $envPath -Parent) -replace '\\','/';" ^
  "$updates = @{ 'DH1_S3_REGION' = '%REGION%'; 'DH1_NAVMESH_CACHE_PATH' = ($repoRoot + '/Shared/NavMesh/Downloaded/world_%WORLD_SERVER_ID%.bin') };" ^
  "$lines = Get-Content -Path $envPath;" ^
  "foreach ($k in $updates.Keys) { $v = $updates[$k]; $prefix = $k + '='; $idx = -1; for ($i = 0; $i -lt $lines.Count; $i++) { if ($lines[$i].StartsWith($prefix)) { $idx = $i; break } }; if ($idx -ge 0) { $lines[$idx] = $k + '=' + $v } else { $lines += ($k + '=' + $v) } };" ^
  "Set-Content -Path $envPath -Value $lines -Encoding UTF8"
if errorlevel 1 (
    echo [Error] Failed to update .env file.
    exit /b 1
)

set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" (
    echo [Error] SetupS3NavMesh failed. ExitCode=%EXIT_CODE%
    exit /b %EXIT_CODE%
)

echo.
echo [Done] S3 NavMesh setup completed.
echo   Bucket      : %BUCKET_NAME%
echo   Region      : %REGION%
echo   S3 URI      : %S3_URI%
echo   MapCode     : %MAP_CODE%
echo   Updated .env: %ENV_PATH%
echo.
echo Next:
echo   1^) Ensure DB row exists in world_navmesh_source ^(world_server_id=%WORLD_SERVER_ID%, map_code=%MAP_CODE%^)
echo   2^) Restart WorldServer
echo   3^) Check logs for 'NavMesh downloaded from S3 URI'
exit /b 0
