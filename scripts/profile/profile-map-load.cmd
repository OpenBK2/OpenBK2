@echo off
rem Profile one map load: start a trace, launch the game on the map with
rem --exit-after-load, stop the trace when it exits, and reduce it to a hot list.
rem
rem Run from an elevated prompt: ETW kernel sampling is not available otherwise.
rem
rem   profile-map-load.cmd Custom/Missions/Razvedka/MapInfo.xdb
rem   profile-map-load.cmd "Custom/Missions/Mission Impossible/mapinfo.xdb" --mod MPBMod6
rem   profile-map-load.cmd <map> --setvar deep_map_load=1 --name deep
rem   profile-map-load.cmd --help
rem
rem The captures folder next to this script gets <name>.etl, the reports and
rem <name>-hot.txt, which is the one to share. profile_map_load.py says what each is.

setlocal

net session >nul 2>&1
if errorlevel 1 goto :needadmin

where python >nul 2>&1
if errorlevel 1 goto :nopython

python "%~dp0profile_map_load.py" %*
set "RESULT=%ERRORLEVEL%"
rem Launched from Explorer, the window would close before the output can be read.
if "%~1"=="" pause
exit /b %RESULT%

:needadmin
echo.
echo This needs administrator rights: ETW kernel tracing is not available otherwise.
echo Open an elevated prompt and run it from there, with the map to load:
echo   profile-map-load.cmd Custom/Missions/Razvedka/MapInfo.xdb
echo.
exit /b 1

:nopython
echo.
echo python is not on PATH, and it runs the whole capture.
echo.
exit /b 1
