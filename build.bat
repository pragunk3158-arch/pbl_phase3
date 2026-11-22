@echo off
echo Compiling SmartWaste...
echo.

g++ -std=c++17 -Wall -Wextra -Iinclude src/Bin.cpp src/Vehicle.cpp src/Graph.cpp src/Route.cpp src/WasteManager.cpp src/main.cpp -o SmartWaste.exe

if %errorlevel% neq 0 (
    echo.
    echo ============================================================
    echo   COMPILATION FAILED
    echo   If the error says 'g++ is not recognized', MinGW is not
    echo   installed or it is not in the PATH.
    echo ============================================================
    pause
    exit /b 1
)

echo.
echo Done. SmartWaste.exe is ready.
