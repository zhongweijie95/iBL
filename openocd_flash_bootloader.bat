@echo off
chcp 65001 > nul 2>&1
title openocd_flash_bootloader_AT32F403A

:: ====================================== Configuration Section (Modify as needed) ======================================
:: 1. OpenOCD command (If OpenOCD is added to system environment variables, keep "openocd"; otherwise, fill in the full path, e.g., "D:\openocd\bin\openocd.exe")
set "OPENOCD_CMD=openocd"

:: 2. Configuration file path (%~dp0 indicates the directory of the batch file, ensuring an absolute path to avoid relative path issues)
set "CFG_FILE=%~dp0openocd.cfg"

:: 3. Bin file path configuration (Dual path search: subdirectory first, then batch file directory as fallback)
set "BIN_FILE_SUB=%~dp0build\MinSizeRel\AT32F403A_BL.bin"  :: Subdirectory path (original path)
set "BIN_FILE_ROOT=%~dp0AT32F403A_BL.bin"                  :: Batch file sibling directory path
set "BIN_FILE="  :: Initialize the final bin file path variable

:: 4. Flash start address (Default Flash start address for AT32F403A is 0x08000000. Modify if your Bootloader uses a custom start address)
set "FLASH_ADDR=0x08000000"

:: ====================================== Pre-checks ======================================
:: Check if openocd.cfg exists
if not exist "%CFG_FILE%" (
    echo [Error] Configuration file not found: %CFG_FILE%
    echo Please ensure openocd.cfg is in the same directory as this batch file.
    goto :EXIT_WITH_PAUSE  :: Jump to unified pause exit logic
)

:: Dual path search for Bin file (Priority: subdirectory > sibling directory)
if exist "%BIN_FILE_SUB%" (
    set "BIN_FILE=%BIN_FILE_SUB%"
    echo [Info] Found Bin file in subdirectory: %BIN_FILE%
) else if exist "%BIN_FILE_ROOT%" (
    set "BIN_FILE=%BIN_FILE_ROOT%"
    echo [Info] Bin file not found in subdirectory, found in sibling directory: %BIN_FILE%
) else (
    echo [Error] AT32F403A_BL.bin not found. Checked the following two paths:
    echo 1. Subdirectory: %BIN_FILE_SUB%
    echo 2. Sibling directory: %BIN_FILE_ROOT%
    goto :EXIT_WITH_PAUSE  :: Jump to unified pause exit logic
)

:: ====================================== Core Modification: Replace \ with / in paths ======================================
:: Replace backslash (\) with forward slash (/) for OpenOCD compatibility (cross-platform support)
set "CFG_FILE=%CFG_FILE:\=/%"
set "BIN_FILE=%BIN_FILE:\=/%"

:: ====================================== Start Flashing Process ======================================
echo ====================================== Starting Flashing Process ======================================
echo Configuration File: %CFG_FILE%
echo Bin File: %BIN_FILE%
echo Flash Address: %FLASH_ADDR%
echo ----------------------------------------------------------------------------------------

:: Call OpenOCD to execute flashing commands
:: Core command explanation:
:: -f Load configuration file
:: -c "program" Simplified flashing command (erase + write + verify + reset + exit)
:: - Forward slash (/) in paths is fully supported by OpenOCD, no escape needed
"%OPENOCD_CMD%" -f "%CFG_FILE%" -c "program %BIN_FILE% %FLASH_ADDR% verify reset exit;"

:: ====================================== Unified Exit with Pause ======================================
:EXIT_WITH_PAUSE
echo ====================================== Operation Completed ======================================
echo Please press any key to exit the window...
pause > nul  :: Pause and hide the default "Press any key to continue . . ." prompt
exit /b