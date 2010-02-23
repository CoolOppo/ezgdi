%echo off

set NEW=%1%
set OLD=%2%
set DIR=..\releases
set NEWDIR=%DIR%\ezgdi-%NEW%
set OLDDIR=%DIR%\ezgdi-%OLD%
set OLDFILE=%OLDDIR%\ezgdi-x86.dll
set NEWFILE=%NEWDIR%\ezgdi-x64-uninstall.reg
set DEBUGDIR=%DIR%\debug

if not exist %NEWDIR% goto nonewdir
if not exist %NEWFILE% goto badnewdir

echo %NEWDIR% already created, now update it.
goto doupdate

:nonewdir
if not exist %OLDFILE% goto noolddir
if not exist %DEBUGDIR% goto nodebugdir

echo make release version %NEW% based on %OLD%.
copy /s %OLDDIR% %NEWDIR%

:doupdate
echo copy ezgdi and easyhook dlls to %NEWDIR%.
copy ezgdi-x86.dll %NEWDIR%\
copy ezgdi-x64.dll %NEWDIR%\
copy easyhook32.dll %NEWDIR%\
copy easyhook64.dll %NEWDIR%\

echo copy ezgdi debug symbol files to %DEBUGDIR%.
copy projects\ezgdi-dll-win32-RELEASE\vc90.pdb %DEBUGDIR%\ezgdi-x86-%NEW%.pdb
copy projects\ezgdi-dll-x64-RELEASE\vc90.pdb %DEBUGDIR%\ezgdi-x64-%NEW%.pdb

echo release directory %NEW% is updated.
echo Please remember to edit ChangeLog.txt before uploading.
goto end

:noolddir
echo %OLDFILE% not exist
echo Please specify a valid version number in the 2nd argument.
goto end

:nodebugdir
echo %OLDFILE% not exist, please create %OLDFILE% yourself.
goto end

:badnewdir
echo %NEWDIR% exists, but not a valid release dir
echo Please specify a valid version number in the 1st agument.
goto end

:end