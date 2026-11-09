@echo off
rem Fetch everything the build needs into toolkit\ (run once; safe to re-run). Nothing is installed and nothing is
rem written outside this folder.
rem
rem   setup.cmd
rem
rem Unpacks the portable Python 3.12 (python.org's embeddable zip) into toolkit\python with Windows' own curl.exe and
rem tar.exe, then runs tools\setup_toolkit.py with it: 7-Zip (unpacked from its installer, never run), the VC6
rem compiler packages and the SDK files. About 1.2 GB of downloads, cached in toolkit\dl.
setlocal
set "ROOT=%~dp0"
set "TK=%ROOT%toolkit"
set "PY_ZIP=python-3.12.10-embed-amd64.zip"
set "PY_URL=https://www.python.org/ftp/python/3.12.10/%PY_ZIP%"
set "PY_SHA=4acbed6dd1c744b0376e3b1cf57ce906f9dc9e95e68824584c8099a63025a3c3"

if exist "%TK%\python\python.exe" goto have_python
call :fetch "%PY_URL%" "%TK%\dl\%PY_ZIP%" %PY_SHA% || exit /b 1
mkdir "%TK%\python" 2>nul
"%SystemRoot%\System32\tar.exe" -xf "%TK%\dl\%PY_ZIP%" -C "%TK%\python" || exit /b 1
:have_python
echo python: ok (%TK%\python)

"%TK%\python\python.exe" -I "%ROOT%tools\setup_toolkit.py" || exit /b 1
exit /b 0

rem :fetch URL FILE SHA256 - download FILE once, then check it against the pinned digest
:fetch
if exist "%~2" goto fetch_check
mkdir "%~dp2" 2>nul
echo downloading %~1
"%SystemRoot%\System32\curl.exe" -fL --retry 3 -o "%~2.part" "%~1" || exit /b 1
move /y "%~2.part" "%~2" >nul || exit /b 1
:fetch_check
set "GOT="
for /f "skip=1 delims=" %%h in ('certutil -hashfile "%~2" SHA256') do if not defined GOT set "GOT=%%h"
set "GOT=%GOT: =%"
if /i not "%GOT%"=="%~3" (
    echo %~nx2: sha256 %GOT% does not match the pinned %~3
    exit /b 1
)
exit /b 0
