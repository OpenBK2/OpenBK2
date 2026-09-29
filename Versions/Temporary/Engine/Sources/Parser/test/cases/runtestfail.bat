testparsing.exe %1 1
GOTO a%ERRORLEVEL%

:a0
echo test %1: fail!
GOTO end

:a1
echo test %1: ok!

:end
