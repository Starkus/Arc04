#/bin/bash
common_compiler="-std=c++17 -Igen -Iexternal/include -W -Wno-unused-value -Wno-missing-field-initializers
-Wno-switch"
common_linker="-lm"
debug_options="-O0 -g -DDEBUG_BUILD=1"
release_options="-O3"

# Build preprocessor
clang -o bin/Preprocessor tools/Preprocessor.cpp $common_compiler $debug_options $common_linker -Isrc -ldl

if [ -n "$1" ] && [ $1 == '-r' ]
then
	echo Building release

	clang -o gen/LinuxPlatform.i src/LinuxPlatform.cpp $common_compiler $release_options -DPREPROCESSING=1 --preprocess
	bin/Preprocessor -platform gen/LinuxPlatform.i > /dev/null
	clang -o bin/LinuxPlatform gen/LinuxPlatform.cpp $common_compiler $release_options $common_linker -lX11 -lGL -lGLU -ldl

	clang -o gen/Game.i src/Game.cpp $common_compiler $release_options -DPREPROCESSING=1 --preprocess
	bin/Preprocessor -game gen/Game.i > /dev/null
	clang -o bin/Game.so gen/Game.cpp $common_compiler $release_options $common_linker -fPIC -shared

	clang -o bin/Bakery tools/Bakery.cpp $common_compiler $release_options $common_linker -Isrc -ldl -lstdc++
else
	echo Building debug

	clang -o gen/LinuxPlatform.i src/LinuxPlatform.cpp $common_compiler $debug_options -DPREPROCESSING=1 --preprocess
	bin/Preprocessor -platform gen/LinuxPlatform.i > /dev/null
	clang -o bin/LinuxPlatform_debug gen/LinuxPlatform.cpp $common_compiler $debug_options $common_linker -lX11 -lGL -lGLU -ldl

	clang -o gen/Game.i src/Game.cpp $common_compiler $debug_options -DPREPROCESSING=1 --preprocess
	bin/Preprocessor -game gen/Game.i > /dev/null
	clang -o bin/Game_debug.so gen/Game.cpp $common_compiler  $debug_options $common_linker -fPIC -shared

	clang -o bin/Bakery_debug tools/Bakery.cpp $common_compiler $debug_options $common_linker -Isrc -ldl -lstdc++
fi

ctags -R .
