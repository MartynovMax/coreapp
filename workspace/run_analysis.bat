@echo off
:: Run the analysis CLI tool from the workspace/ directory.
:: Usage examples:
::
::   report:
::     run_analysis.bat report --input runs\article1\20260419_143000
::
::   compare:
::     run_analysis.bat compare --baseline runs\run1 --candidate runs\run2
::
::   plot:
::     run_analysis.bat plot --input runs\run1 --kind latency --output latency.png
::
::   help:
::     run_analysis.bat --help

setlocal
set "WORK_DIR=%~dp0"
cd /d "%WORK_DIR%"

:: Python analysis module lives in ../benchmarks/analysis
set "PYTHONPATH=%WORK_DIR%..\benchmarks"
python -m analysis %* 2>&1


