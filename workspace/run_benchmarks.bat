@echo off
:: Run the benchmark executable from the workspace/ directory.
:: All relative paths (config/, runs/) resolve from here.
::
:: Usage examples:
::
::   Basic run (text output):
::     run_benchmarks.bat
::
::   With structured output:
::     run_benchmarks.bat --format=all --out=runs\manual\run1
::
::   Filter specific experiments:
::     run_benchmarks.bat --filter=article1/malloc/*
::
::   With custom config:
::     run_benchmarks.bat --config=config\article1_matrix.json --format=all --out=runs\run1
::
::   Full dataset run (all outputs + manifest):
::     run_benchmarks.bat --format=all --out=runs\run1 --repetitions=10
::
:: The executable is searched in common build output locations.
:: Override with: set BENCH_EXE=<path>

setlocal

:: Change to the workspace/ directory so relative paths work
set "WORK_DIR=%~dp0"
cd /d "%WORK_DIR%"

:: Locate the executable (search common build output paths)
if defined BENCH_EXE goto :run

:: Try build presets in priority order (exe is one level up in build/)
for %%P in (
    "..\build\bench\benchmarks\coreapp_benchmarks.exe"
    "..\build\benchmarks\Release\coreapp_benchmarks.exe"
    "..\build\benchmarks\Debug\coreapp_benchmarks.exe"
    "..\cmake-build-debug\benchmarks\coreapp_benchmarks.exe"
    "..\cmake-build-release\benchmarks\coreapp_benchmarks.exe"
) do (
    if exist "%%P" (
        set "BENCH_EXE=%%P"
        goto :run
    )
)

echo ERROR: Could not find coreapp_benchmarks.exe
echo Build first with build_benchmarks.bat, or set BENCH_EXE=^<path^>
exit /b 1

:run
echo [run_benchmarks] Working dir: %CD%
echo [run_benchmarks] Executable:  %BENCH_EXE%
echo.
"%BENCH_EXE%" %*


