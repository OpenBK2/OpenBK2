@echo off
rem Start a CPU sampling trace.
rem
rem Run this once the game is already in the scene you want measured, not before
rem launching it. ETW tracing is system wide and independent of any process, so nothing
rem has to be started under a profiler and none of the executable loading, the menu or
rem the mission load ends up in the profile.
rem
rem Right click this file and pick "Run as administrator": the kernel sampling provider
rem is not available otherwise. Double clicking without elevating just prints a reminder.
rem
rem   start-profile.cmd            sample at the 1000 Hz default
rem   start-profile.cmd 4000       sample at 4000 Hz, for resolving short functions

setlocal

net session >nul 2>&1
if errorlevel 1 goto :needadmin

set "RATE=%~1"
if "%RATE%"=="" set "RATE=1000"

set "XPERF=%ProgramFiles(x86)%\Windows Kits\10\Windows Performance Toolkit\xperf.exe"

rem xperf takes the interval in 100ns units, so 10000000 / Hz. Only integer maths is
rem available here, which is fine for the rates anyone would pick.
set /a INTERVAL=10000000 / %RATE%

if exist "%XPERF%" (
    "%XPERF%" -setprofint %INTERVAL% >nul
    echo sampling at %RATE% Hz
) else (
    echo xperf not found, leaving the sampling interval at the system default
)

rem Clear anything an interrupted session left behind, then start. CPU is the built-in
rem profile: the sampled profile provider plus the process and image events needed to
rem turn an address into a module and a function.
wpr -cancel >nul 2>&1
wpr -start CPU -filemode
if errorlevel 1 goto :failed

echo.
echo recording.
echo   play the scene you want measured, then run stop-profile.cmd
echo   20 to 30 seconds of steady action is the sweet spot: long enough for a stable
echo   hot list, short enough to keep the trace and the export manageable.
echo.
pause
exit /b 0

:needadmin
echo.
echo This needs administrator rights: ETW kernel tracing is not available otherwise.
echo Right click start-profile.cmd and choose "Run as administrator".
echo.
pause
exit /b 1

:failed
echo.
echo wpr failed to start. If it says a trace is already running, run:  wpr -cancel
echo.
pause
exit /b 1
