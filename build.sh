#/bin/bash
common_compiler="-lm -W -Wno-unused-value"
debug_options="-O0 -g -DDEBUG_BUILD"
clang -o bin/LinuxPlatform src/LinuxPlatform.cpp $common_compiler $debug_options -lX11 -lGL -lGLU -ldl
clang -o bin/Game.so src/Game.cpp $common_compiler  $debug_options -fPIC -shared
clang -o bin/Bakery tools/Bakery.cpp $common_compiler $debug_options -Iexternal/include -Isrc -ldl -lstdc++
