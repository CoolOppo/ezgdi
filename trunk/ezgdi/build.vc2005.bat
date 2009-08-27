@echo off

setlocal

set INCLUDE=..\..\freetype2\include;..\..\detours\include;%INCLUDE%
set LIB=..\..\freetype2\objs;..\..\detours\lib;%LIB%

:go
nmake -f Makefile.vc2005 %*
if errorlevel 1 goto err
goto end

:err
pause
goto go

:end
endlocal
rem pause
