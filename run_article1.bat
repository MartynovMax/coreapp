@echo off
setlocal

echo ==============================================================
echo   Article 1 - Full Experiment Matrix Run
echo ==============================================================
echo.

:: ----------------------------------------------------------------
:: Configuration
:: ----------------------------------------------------------------
set CONFIG=benchmarks\config\article1_matrix.json
set MEASUREMENTS=timer,counter
set FORMAT=all

:: ----------------------------------------------------------------
:: Find executable
:: Prefer Ninja bench build (has all CLI flags), then VS builds
:: ----------------------------------------------------------------
if exist "build\bench\benchmarks\coreapp_benchmarks.exe" (
    set EXE=build\bench\benchmarks\coreapp_benchmarks.exe
    echo Using Bench build (Ninja, RelWithDebInfo^)
) else if exist "build\benchmarks\benchmarks\Release\coreapp_benchmarks.exe" (
    set EXE=build\benchmarks\benchmarks\Release\coreapp_benchmarks.exe
    echo Using VS Release build
) else if exist "build\benchmarks\benchmarks\Debug\coreapp_benchmarks.exe" (
    set EXE=build\benchmarks\benchmarks\Debug\coreapp_benchmarks.exe
    echo WARNING: Using Debug build - results will be slower than Release!
) else (
    echo ERROR: coreapp_benchmarks.exe not found.
    echo.
    echo Build with Ninja (recommended^):
    echo   cmake --preset bench
    echo   cmake --build build\bench
    echo.
    echo Or with Visual Studio:
    echo   build_benchmarks.bat
    exit /b 1
)

:: ----------------------------------------------------------------
:: Verify the executable supports required flags
:: ----------------------------------------------------------------
%EXE% --help 2>&1 | findstr /C:"--config" >nul
if %errorlevel% neq 0 (
    echo.
    echo ERROR: Selected executable does not support --config flag.
    echo        It may be an older build. Please rebuild:
    echo          cmake --preset bench
    echo          cmake --build build\bench
    exit /b 1
)

echo Executable: %EXE%
echo Config:     %CONFIG%
echo.

:: ----------------------------------------------------------------
:: Dry run first (show what will be executed)
:: ----------------------------------------------------------------
echo --- Dry Run (planned scenarios) ---
echo.
%EXE% --config=%CONFIG% --measurements=%MEASUREMENTS% --format=%FORMAT% --batch --dry-run
echo.

:: ----------------------------------------------------------------
:: Ask user to confirm
:: ----------------------------------------------------------------
set /p CONFIRM="Run all scenarios? (y/n): "
if /i not "%CONFIRM%"=="y" (
    echo Cancelled.
    exit /b 0
)

echo.
echo --- Starting batch execution ---
echo.

:: ----------------------------------------------------------------
:: Full batch run
:: ----------------------------------------------------------------
%EXE% --config=%CONFIG% --measurements=%MEASUREMENTS% --format=%FORMAT% --batch

set EXIT_CODE=%errorlevel%

echo.
echo ==============================================================
if %EXIT_CODE%==0 (
    echo   All scenarios completed successfully!
) else if %EXIT_CODE%==3 (
    echo   Partial failure: some scenarios failed.
) else (
    echo   Batch failed with exit code %EXIT_CODE%
)
echo   Results are in: runs\article1\
echo ==============================================================
echo.

pause
exit /b %EXIT_CODE%
