@echo off
rem Put every capture in the captures folder side by side.
rem
rem Needs no arguments and no elevation: this only reads reports that
rem stop-profile.cmd already produced.
rem
rem Capture a few different scenes first, naming each one, for example:
rem   stop-profile.cmd plains      open ground, many tanks moving
rem   stop-profile.cmd forest      dense terrain
rem   stop-profile.cmd orders      recording started before a mass move order
rem   stop-profile.cmd idle        paused, as a baseline

setlocal
set "HERE=%~dp0"

where python >nul 2>&1
if errorlevel 1 goto :nopython

python "%HERE%compare_profiles.py" --out "%HERE%captures\compare.txt"
if errorlevel 1 goto :failed

echo.
type "%HERE%captures\compare.txt"
echo.
echo saved to %HERE%captures\compare.txt
echo.
pause
exit /b 0

:nopython
echo.
echo python is not on PATH.
echo.
pause
exit /b 1

:failed
echo.
echo Nothing to compare. Run stop-profile.cmd at least once first.
echo.
pause
exit /b 1
