@echo off
SETLOCAL

REM Define project directories
SET "PROJECT_DIR=%~dp0"
SET "BUILD_VERSION=v1.0.0"  
REM Change this to your actual version directory
SET "BUILD_DIR=%PROJECT_DIR%build\%BUILD_VERSION%"
SET "DIST_DIR=%PROJECT_DIR%dist"

REM Create dist directory if it does not exist
if not exist "%DIST_DIR%" (
    mkdir "%DIST_DIR%"
)

REM Path to esptool.exe. If esptool is in PATH, simply use "esptool"
SET "ESPTOOL_CMD=esptool.exe"

REM Define bin files with memory addresses
SET BIN_FILES=(
    "0x1000 %BUILD_DIR%\AirMonitor_Feather.ino.bootloader.bin"
    "0x8000 %BUILD_DIR%\AirMonitor_Feather.ino.partitions.bin"
    "0xe000 %PROJECT_DIR%build\boot_app0.bin"
    "0x10000 %BUILD_DIR%\AirMonitor_Feather.ino.bin"
    "0x670000 %PROJECT_DIR%build\config.bin"
)

REM Check if all bin files exist
FOR %%I IN (
    "0x1000 %BUILD_DIR%\AirMonitor_Feather.ino.bootloader.bin"
    "0x8000 %BUILD_DIR%\AirMonitor_Feather.ino.partitions.bin"
    "0xe000 %PROJECT_DIR%build\boot_app0.bin"
    "0x10000 %BUILD_DIR%\AirMonitor_Feather.ino.bin"
    "0x670000 %PROJECT_DIR%build\config.bin"
) DO (
    FOR /F "tokens=1,* delims= " %%A IN (%%I) DO (
        IF NOT EXIST "%%B" (
            echo Error: Binary file not found at %%B
            EXIT /B 1
        )
    )
)

REM Define output merged binary path
SET "MERGED_BIN=%DIST_DIR%\merged.bin"

REM Construct the merge command
SET "MERGE_CMD=%ESPTOOL_CMD% merge_bin --output "%MERGED_BIN%" --flash_mode dio --flash_freq 80m --flash_size 8MB"

FOR %%I IN (
    "0x1000 %BUILD_DIR%\AirMonitor_Feather.ino.bootloader.bin"
    "0x8000 %BUILD_DIR%\AirMonitor_Feather.ino.partitions.bin"
    "0xe000 %PROJECT_DIR%build\boot_app0.bin"
    "0x10000 %BUILD_DIR%\AirMonitor_Feather.ino.bin"
    "0x670000 %PROJECT_DIR%build\config.bin"
) DO (
    FOR /F "tokens=1,* delims= " %%A IN (%%I) DO (
        SET "MERGE_CMD=!MERGE_CMD! %%A %%B"
    )
)

REM Enable delayed expansion
SETLOCAL EnableDelayedExpansion

REM Execute the merge command
echo Executing merge command:
echo !MERGE_CMD!
!MERGE_CMD!
IF ERRORLEVEL 1 (
    echo Error during merging binaries.
    EXIT /B 1
)

echo Merged binary created at: %MERGED_BIN%
echo Bin merging completed successfully.

ENDLOCAL
ENDLOCAL
