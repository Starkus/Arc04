@echo off

cls

set SourceFiles=bakery.cpp
set CompilerFlags=-MTd -nologo -Gm- -GR- -Od -Oi -EHa- -W4 -wd4201 -wd4100 -wd4996 -DDEBUG_BUILD=1 -FC -Z7 -I"..\\external\\include\\tinyxml" -I"..\\external\\include\\SDL" -I"..\\external\\include" -I"..\\src"
set LinkerFlags=-opt:ref -incremental:no -SUBSYSTEM:CONSOLE -LIBPATH:"..\\external\\lib"
set Libraries=user32.lib Gdi32.lib winmm.lib shell32.lib SDL2main.lib SDL2.lib

pushd .\build

echo BUILDING BAKERY CODE
set start=%time%
cl %CompilerFlags% %SourceFiles% %Libraries% -link %LinkerFlags%
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
