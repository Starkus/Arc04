@echo off

cls

set SourceFiles=..\src\Game.cpp
set CompilerFlags=-nologo -Z7
set LinkerFlags=-LIBPATH:../lib -SUBSYSTEM:WINDOWS
set Libraries=shell32.lib sdl2.lib sdl2main.lib

IF NOT EXIST .\build mkdir .\build

pushd .\bin

echo BUILDING SECRET GAME
set start=%time%
cl %CompilerFlags% %SourceFiles% -link %Libraries% %LinkerFlags%
set end=%time%
IF %ERRORLEVEL% NEQ 0 echo Plaftorm code compilation [31mfailed![0m
IF %ERRORLEVEL% EQU 0 echo Platform code compilation [32msuccess[0m

popd

cmd /c timediff.bat Compile %start% %end%
echo.

echo Generating tags...
ctags -R --use-slash-as-filename-separator=yes .
IF %ERRORLEVEL% NEQ 0 echo Tag generation [31mfailed![0m
IF %ERRORLEVEL% EQU 0 echo Done generating tags
