@echo off

REM Usage: runtest cl|gcc testname testarg ...

if EXIST %2.pass goto bypass

REM Make sure we start with only those files we expect to need
if exist tmp\*.* echo y | erase tmp\*.* > nul:
rmdir tmp
mkdir tmp

copy ..\pthread.dll tmp > nul:
copy ..\pthread.h tmp > nul:
copy ..\semaphore.h tmp > nul:
copy ..\sched.h tmp > nul:
copy test.h tmp > nul:
copy ..\pthread.lib tmp > nul:
copy ..\libpthread32.a tmp > nul:

cd tmp

REM Compile the test case
REM  produces aout.exe using the compiler given as %1
call ..\c%1.bat %2 > ..\%2.%1log

if ERRORLEVEL 1 goto cleanup

REM erase ..\%2.%1log

echo TEST: %2 [%1]

REM Run the test case
aout.exe %3 %4 %5 %6 %7 %8 %9

set RESULT=%ERRORLEVEL%

if %RESULT% EQU 0 echo Passed [%RESULT%] > ..\%2.pass

:cleanup

cd ..

REM Clean up
if exist tmp\*.* echo y | erase tmp\*.* > nul:

if EXIST %2.pass echo Passed [%RESULT%]

:bypass