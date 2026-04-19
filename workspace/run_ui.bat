@echo off
:: Launch the benchmark analysis desktop UI.
::
:: Requirements:
::   pip install -r ..\benchmarks\analysis\requirements.txt

setlocal
set "WORK_DIR=%~dp0"
set "PYTHONPATH=%WORK_DIR%..\benchmarks"
python "%WORK_DIR%..\benchmarks\analysis\ui.py"


