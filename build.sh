#/bin/bash
common_compiler="-Igen -Iexternal/include -lm -W -Wno-unused-value"
debug_options="-O0 -g -DDEBUG_BUILD"

# Build preprocessor
clang -o bin/Preprocessor tools/Preprocessor.cpp $common_compiler $debug_options -Isrc -ldl
bin/Preprocessor src/LinuxPlatform.cpp

if [ -n "$1" ] && [ $1 == '-r' ]
then
	echo Building release
	clang -o bin/LinuxPlatform src/LinuxPlatform.cpp $common_compiler -lX11 -lGL -lGLU -ldl
	clang -o bin/Game.so src/Game.cpp $common_compiler  -fPIC -shared
	clang -o bin/Bakery tools/Bakery.cpp $common_compiler -Isrc -ldl -lstdc++
else
	echo Building debug
	clang -o bin/LinuxPlatform_debug src/LinuxPlatform.cpp $common_compiler $debug_options -lX11 -lGL -lGLU -ldl
	clang -o bin/Game_debug.so src/Game.cpp $common_compiler  $debug_options -fPIC -shared
	clang -o bin/Bakery_debug tools/Bakery.cpp $common_compiler $debug_options -Isrc -ldl -lstdc++
fi

ctags -R .
