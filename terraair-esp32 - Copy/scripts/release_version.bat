REM Script just uploads the bin to the utils server must have keys preconfigured.
@echo off

cls
echo "------------------------------------------"
echo "  _____                      _    _       "
echo " |_   _|__ _ __ _ __ __ _   / \  (_)_ __  "
echo "   | |/ _ \ '__| '__/ _` | / _ \ | | '__| "
echo "   | |  __/ |  | | | (_| |/ ___ \| | |    "
echo "   |_|\___|_|  |_|  \__,_/_/   \_\_|_|    "
echo "                                          "
echo "   Release Utility (c) 2023        v1.0.0 "
echo "------------------------------------------"
echo.

setlocal enabledelayedexpansion

REM Get the new name from the user (excluding .bin extension)
set /p newname="Release number: "

REM Initialize a flag to check if a .bin file was found
set filefound=0

REM Search for .bin files in the ./ folder
for /r "./" %%f in (*.bin) do (
    REM Set the filefound flag to 1
    set filefound=1

    REM Save the directory path of the file
    set dirpath=%%~dpf
    
    REM Rename the file
    ren "%%f" "!newname!.bin"
    
    REM Form the new path after renaming
    set newfilepath=!dirpath!!newname!.bin

    REM scp the file to the server
    scp "!newfilepath!" mfiot@airmonitor-utils.terrasls.com:/var/www/airmonitor-utils.terrasls.com/softwareupdate/feather/
    scp "!newfilepath!" mfiot@airmonitor-utils.terrasls.com:/var/www/airmonitor-utils.terrasls.com/softwareupdate/feather/latest.bin

    REM Check if scp was successful and then delete the file?
    if !errorlevel! == 0 (
        del "!newfilepath!"
    ) else (
        echo Failed to transfer the file to the server.
    )
)

REM Check the filefound flag and inform the user if no .bin file was found
if !filefound! == 0 (
    echo No .bin file found in the ./ folder.
)

endlocal

pause