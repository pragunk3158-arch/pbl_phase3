@echo off
cd /d "%~dp0"

if not exist SmartWaste.exe call build.bat
if not exist SmartWaste.exe (
    echo Cannot start the dashboard without SmartWaste.exe
    pause
    exit /b 1
)

echo.
echo Starting the dashboard...
echo If the page looks old or empty, press Ctrl+F5 in the browser.
echo.

python server.py
if %errorlevel% neq 0 py server.py

pause
