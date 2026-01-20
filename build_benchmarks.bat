@echo off
setlocal

echo ================================
echo   Building coreapp benchmarks
echo ================================
echo.

if not exist build mkdir build
if not exist build\benchmarks (
    echo Creating build\benchmarks directory...
    mkdir build\benchmarks
)

echo Running CMake configuration with BUILD_BENCHMARKS=ON...
cmake -S . -B build\benchmarks -G "Visual Studio 17 2022" -A x64 -DBUILD_BENCHMARKS=ON

if %errorlevel% neq 0 (
    echo.
    echo !!! CMake configuration FAILED !!!
    exit /b %errorlevel%
)

echo.
echo Building benchmarks...
cmake --build build\benchmarks --config Debug --target coreapp_benchmarks

if %errorlevel% neq 0 (
    echo.
    echo !!! Build FAILED !!!
    exit /b %errorlevel%
)

echo.
echo =========================================
echo   Build completed successfully!
echo   Executable: build\benchmarks\Debug\coreapp_benchmarks.exe
echo =========================================
echo.
echo Opening Visual Studio solution...
start build\benchmarks\coreapp.sln
echo.
echo Run with: build\benchmarks\Debug\coreapp_benchmarks.exe --help
echo.

pause
