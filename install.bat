@echo off
setlocal EnableDelayedExpansion

echo Stellarium Moon Avoidance Plugin Installer
echo ==========================================

set "DEFAULT_DIR=%APPDATA%\Stellarium"
set "TARGET_DIR="

:: Check if default directory exists
if exist "%DEFAULT_DIR%" (
    echo Found Stellarium data directory at: %DEFAULT_DIR%
    set /p "USE_DEFAULT=Install to this location? [Y/n]: "
    if /i "!USE_DEFAULT!" neq "n" (
        set "TARGET_DIR=%DEFAULT_DIR%"
    )
)

:: If not using default, ask user
if not defined TARGET_DIR (
    :ASK_DIR
    echo.
    echo Please enter the full path to your Stellarium User Data directory.
    echo (e.g. C:\Users\YourName\AppData\Roaming\Stellarium)
    set /p "TARGET_DIR=Path: "
    
    if not exist "!TARGET_DIR!" (
        echo Directory not found: "!TARGET_DIR!"
        goto ASK_DIR
    )
)

echo.
echo Installing to: !TARGET_DIR!

set "MODULE_DIR=!TARGET_DIR!\modules\MoonAvoidance"

if not exist "!MODULE_DIR!" (
    echo Creating directory: !MODULE_DIR!
    mkdir "!MODULE_DIR!"
)

echo Copying files...
copy /Y "MoonAvoidance.dll" "!MODULE_DIR!\"
if %ERRORLEVEL% NEQ 0 goto ERROR
copy /Y "plugin.ini" "!MODULE_DIR!\"
if %ERRORLEVEL% NEQ 0 goto ERROR

echo.
echo Installation successful!
echo Please restart Stellarium and enable the plugin in the Configuration window (F2) -> Plugins.
pause
exit /b 0

:ERROR
echo.
echo An error occurred during installation.
pause
exit /b 1
