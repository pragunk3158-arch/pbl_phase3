@echo off
cd /d "%~dp0"
if not exist SmartWaste.exe call build.bat
if not exist SmartWaste.exe exit /b 1
SmartWaste.exe
pause
