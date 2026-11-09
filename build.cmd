@echo off
rem Build the game's images into target\ with the portable toolchain in toolkit\ (run setup.cmd once first).
rem
rem   build.cmd [-j N] [--compile-only] [target ...]
rem
rem Targets (default: all): tcp mixer default lle dx7hr dx7hrtnl language exe. Staging and temporary files go to
rem workdir\; nothing is written outside this folder. All options: build.cmd --help
setlocal
set "PY=%~dp0toolkit\python\python.exe"
if not exist "%PY%" (
    echo toolkit\python is missing: run setup.cmd first
    exit /b 1
)
"%PY%" -I "%~dp0build.py" %*
