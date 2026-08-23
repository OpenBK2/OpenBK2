@echo off
rem Stop the trace, resolve symbols, and reduce it to something readable.
rem
rem Right click and pick "Run as administrator", same as start-profile.cmd.
rem
rem   stop-profile.cmd                       name it "profile", process Game.exe
rem   stop-profile.cmd battle                name it "battle"
rem   stop-profile.cmd battle Game.exe       name it, and pick the process explicitly
rem
rem Everything lands in the captures folder next to this script:
rem   <name>.etl              raw trace, opens in WPA when a human wants to dig
rem   <name>-butterfly.html   full report with callers and callees, opens in a browser
rem   <name>-hot.txt          ranked summary, small enough to paste

setlocal

net session >nul 2>&1
if errorlevel 1 goto :needadmin

set "NAME=%~1"
if "%NAME%"=="" set "NAME=profile"
rem Arguments here are positional, not named. Passing -Process, PowerShell style, would
rem otherwise quietly name the capture "-Process".
echo %NAME%| findstr /b /c:"-" >nul && goto :badname

set "PROC=%~2"
if "%PROC%"=="" set "PROC=Game.exe"

set "HERE=%~dp0"
set "OUT=%HERE%captures"
set "XPERF=%ProgramFiles(x86)%\Windows Kits\10\Windows Performance Toolkit\xperf.exe"
set "BUILD=%HERE%..\..\out\build\Windows-x64-Release"

if not exist "%XPERF%" goto :noxperf
if not exist "%OUT%" mkdir "%OUT%"

echo saving to %OUT%
echo stopping trace...
wpr -stop "%OUT%\%NAME%.etl"
if errorlevel 1 goto :failed

rem Local PDBs for the engine and the installed game, the Microsoft server for the
rem kernel, ntdll and the D3D9 runtime. Without the server the kernel frames come out
rem as ***unknown***, which leaves most of the profile unattributed.
set "_NT_SYMBOL_PATH=srv*%TEMP%\symcache*https://msdl.microsoft.com/download/symbols;%BUILD%;C:\Games\bk2\bin"
echo symbols: %_NT_SYMBOL_PATH%

rem -a stack needs an activity. -butterfly aggregates into modules and functions with
rem callers and callees. The alternative, -a dumper, writes one row per frame per sample
rem and would turn a 780 MB trace into many gigabytes of csv.
echo exporting stacks, the first run is slow while symbols download...
"%XPERF%" -i "%OUT%\%NAME%.etl" -o "%OUT%\%NAME%-butterfly.html" -symbols ^
    -a stack -butterfly 20 -process "%PROC%"
if errorlevel 1 goto :failed

where python >nul 2>&1
if errorlevel 1 goto :nopython

python "%HERE%summarize_profile.py" --report "%OUT%\%NAME%-butterfly.html" ^
    --out "%OUT%\%NAME%-hot.txt"
if errorlevel 1 goto :summaryfailed

echo.
echo saved to %OUT%
echo   %NAME%.etl              raw trace, opens in WPA
echo   %NAME%-butterfly.html   full report, opens in a browser
echo   %NAME%-hot.txt          ranked summary   ^<- share this one
echo.
pause
exit /b 0

:needadmin
echo.
echo This needs administrator rights, same as start-profile.cmd.
echo Right click stop-profile.cmd and choose "Run as administrator".
echo.
pause
exit /b 1

:badname
echo.
echo The first argument is the capture name, not a switch. Usage:
echo   stop-profile.cmd [name] [process]
echo   stop-profile.cmd battle Game.exe
echo.
pause
exit /b 1

:noxperf
echo.
echo xperf not found at:
echo   %XPERF%
echo Install the Windows Performance Toolkit, part of the Windows SDK.
echo.
pause
exit /b 1

:nopython
echo.
echo python is not on PATH, which is needed for the ranked summary. The full report is
echo still at %OUT%\%NAME%-butterfly.html and opens in a browser.
echo.
pause
exit /b 1

:summaryfailed
echo.
echo The report could not be summarised. It is still at
echo   %OUT%\%NAME%-butterfly.html
echo and opens in a browser.
echo.
pause
exit /b 1

:failed
echo.
echo wpr or xperf failed. If no trace was running, start one with start-profile.cmd.
echo If it says a trace is already running, clear it with:  wpr -cancel
echo.
pause
exit /b 1
