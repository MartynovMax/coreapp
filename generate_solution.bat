@echo off
setlocal

echo ================================
echo   Generating Visual Studio 2022 solution for coreapp
echo ================================
echo.

REM Clean old build directory (optional)
if exist build\default (
    echo Removing old build\default directory...
    rmdir /s /q build\default
)

echo Creating build\default directory...
if not exist build mkdir build
mkdir build\default

echo Running CMake configuration...
cmake -S . -B build\default -G "Visual Studio 17 2022" -A x64

if %errorlevel% neq 0 (
    echo.
    echo !!! CMake configuration FAILED !!!
    exit /b %errorlevel%
)

cmake --open build\default

echo.
echo =========================================
echo   Solution generated successfully!
echo   Open: build\default\coreapp.sln
echo =========================================

pause
