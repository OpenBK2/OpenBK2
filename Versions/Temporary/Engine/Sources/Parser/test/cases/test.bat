echo off

copy g:\newDB\fastdebug\testparsing.exe testparsing.exe
copy g:\newDB\\fastdebug\*.dll *.dll

for /d %%1 in (*.success) do call runTestSuccess.bat %%1
for /d %%1 in (*.fail) do call runtestfail.bat %%1