@echo off
rem Stop the trace, resolve symbols, and reduce it to something readable.
rem
rem Right click and pick "Run as administrator", same as start-profile.cmd.
rem
rem   stop-profile.cmd                       name it "profile", keep Game.exe
rem   stop-profile.cmd battle                name it "battle"
rem   stop-profile.cmd battle Game.exe       name it, and pick the process explicitly
rem
rem Writes three files into captures\ next to this script:
rem   <name>.etl        raw trace, for opening in WPA when a human wants to dig
rem   <name>.folded     collapsed stacks, the format FlameGraph reads
rem   <name>-hot.txt    ranked functions, small enough to paste into a conversation

setlocal

net session >nul 2>&1
if errorlevel 1 goto :needadmin

set "NAME=%~1"
if "%NAME%"=="" set "NAME=profile"
set "PROC=%~2"
if "%PROC%"=="" set "PROC=Game.exe"

set "HERE=%~dp0"
set "OUT=%HERE%captures"
set "XPERF=%ProgramFiles(x86)%\Windows Kits\10\Windows Performance Toolkit\xperf.exe"
set "BUILD=%HERE%..\..\out\build\Windows-x64-Release"

if not exist "%XPERF%" goto :noxperf
if not exist "%OUT%" mkdir "%OUT%"

echo stopping trace...
wpr -stop "%OUT%\%NAME%.etl"
if errorlevel 1 goto :failed

rem Local PDBs for the engine, the Microsoft server for ntdll, kernel32 and the D3D9
rem runtime. The first export is slow while those download, then they are cached.
set "_NT_SYMBOL_PATH=srv*%TEMP%\symcache*https://msdl.microsoft.com/download/symbols;%BUILD%"
echo symbols: %_NT_SYMBOL_PATH%

echo exporting stacks, the first run is slow while symbols download...
"%XPERF%" -i "%OUT%\%NAME%.etl" -o "%OUT%\%NAME%-stacks.csv" -symbols -a stack
if errorlevel 1 goto :failed

where python >nul 2>&1
if errorlevel 1 goto :nopython

python "%HERE%fold_etw_stacks.py" --csv "%OUT%\%NAME%-stacks.csv" --process "%PROC%" ^
    --folded "%OUT%\%NAME%.folded" --hot "%OUT%\%NAME%-hot.txt"
if errorlevel 1 goto :foldfailed

echo.
echo raw trace : %OUT%\%NAME%.etl
echo collapsed : %OUT%\%NAME%.folded
echo hot list  : %OUT%\%NAME%-hot.txt    ^<- share this one
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
echo python is not on PATH, which is needed to reduce the stack dump.
echo The raw trace is still at %OUT%\%NAME%.etl and opens in WPA.
echo.
pause
exit /b 1

:foldfailed
echo.
echo The stack dump could not be reduced. The message above lists the processes that
echo were in the trace, which is usually enough to work out the right -process name.
echo.
pause
exit /b 1

:failed
echo.
echo wpr or xperf failed. If no trace was running, start one with start-profile.cmd.
echo.
pause
exit /b 1
