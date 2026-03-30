@echo off
:: Launch the benchmark analysis desktop UI.
:: Double-click this file or run it from any directory.
::
:: Requirements:
::   pip install -r benchmarks/analysis/requirements.txt
::
:: Tkinter is included with standard Python — no extra install needed.

:: Resolve the directory where this .bat lives (benchmarks/)
set "SCRIPT_DIR=%~dp0"

python "%SCRIPT_DIR%analysis\ui.py"
