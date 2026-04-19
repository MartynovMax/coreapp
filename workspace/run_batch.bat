@echo off
:: Batch run: execute full article1 experiment matrix with auto-generated output.
::
:: Usage:
::   run_batch.bat                              (default: article1/*, 5 reps)
::   run_batch.bat --repetitions=10             (override repetitions)
::   run_batch.bat --filter=article1/malloc/*   (subset of matrix)
::
:: Output goes to: runs\article1\<YYYYMMDD_HHMMSS>\
::   manifest.json    - batch-level environment manifest
::   summary.csv      - aggregated metrics across all scenarios
::   raw\<scenario>\  - per-scenario outputs (timeseries.jsonl, summary.csv, manifest)

setlocal

set "WORK_DIR=%~dp0"
cd /d "%WORK_DIR%"

echo [batch] Starting full matrix batch run...
echo [batch] Working dir: %CD%
echo.

call run_benchmarks.bat --batch --config=config\article1_matrix.json --filter=article1/* --format=all --measurements=timer,counter %*


