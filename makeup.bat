@echo off

rem set up basic directory
set INSDIR=c:\ezgdi
set BAKDIR=c:\ezgdi\bak

rem check existance
if not exist %INSDIR% goto noinsdir
if not exist %BAKDIR% goto nobakdir

rem set files to copy
set FILES=ezgdi-x86.dll ezgdi-dbg-x86.dll ezgdi-x64.dll ezgdi-dbg-x64.dll EasyHook32.dll EasyHook32d.dll EasyHook64.dll EasyHook64d.dll

rem generate temperary files
set TMPSUFFIX=%date:~0,4%%date:~5,2%%date:~8,2%-%time:~1,1%%time:~3,2%%time:~6,5%

rem backup old files and copy new files
for %%f in (%FILES%) do call :forloop %%f
goto endfor

:forloop
    rem backup old
    move %INSDIR%\%1 %BAKDIR%\%1-%TMPSUFFIX%
    rem copy new
    copy %~dp0%1 %INSDIR%\%1
exit /B

:endfor

goto end

:noinsdir
echo Error: INSDIR: %INSDIR% not exists
goto end

:nobakdir
echo Error: BAKDIR: %BAKDIR% not exists
goto end

:end