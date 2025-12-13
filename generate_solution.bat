@echo off
setlocal

echo ================================
echo   Generating Visual Studio 2022 solution for coreapp
echo ================================
echo.

REM Clean old build directory (optional)
if exist build (
    echo Removing old build directory...
    rmdir /s /q build
)

echo Creating build directory...
mkdir build

echo Running CMake configuration...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

if %errorlevel% neq 0 (
    echo.
    echo !!! CMake configuration FAILED !!!
    exit /b %errorlevel%
)

cmake --open build

echo.
echo =========================================
echo   Solution generated successfully!
echo   Open: build\coreapp.sln
echo =========================================

pause
