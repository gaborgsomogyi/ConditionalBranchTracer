@echo off
set RELEASEDIR=.\release
rmdir /S /Q %RELEASEDIR%
mkdir %RELEASEDIR%

mkdir %RELEASEDIR%\x86
mkdir %RELEASEDIR%\x64

echo "Copying x86 plugin"
copy Win32\Release\ConditionalBranchTracer.dp32 %RELEASEDIR%\x86\

echo "Copying x64 plugin"
copy x64\Release\ConditionalBranchTracer.dp64 %RELEASEDIR%\x64\

echo "Creating commithash.txt"
git rev-parse HEAD > %RELEASEDIR%\commithash.txt

pause
