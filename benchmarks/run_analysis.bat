@echo off
:: Run the analysis CLI tool.
:: Usage examples:
::
::   report:
::     run_analysis.bat report --input results\run1
::     run_analysis.bat report --input results\run1 --allocator tlsf
::
::   compare:
::     run_analysis.bat compare --baseline results\run1 --candidate results\run2
::
::   plot:
::     run_analysis.bat plot --input results\run1 --kind latency --output latency.png
::     run_analysis.bat plot --input results\run1 --kind timeseries --metric-key live_bytes --output ts.png
::
::   help:
::     run_analysis.bat --help
::     run_analysis.bat report --help
::     run_analysis.bat compare --help
::     run_analysis.bat plot --help

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"
python -m analysis %* 2>&1


