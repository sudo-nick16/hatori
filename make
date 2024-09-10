#!/bin/bash


if [ "$1" = "build-web" ]; then
	echo "building the app for web"
	mkdir -p build
	emcc -O0 -o build/index.html src/hatori.cpp -I./src/external/raylib/src -L./lib -l:libraylibweb.a \
	-s USE_GLFW=3 -s ASYNCIFY --shell-file=hatori.html -s LINKABLE=1 -s EXPORT_ALL=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 \
	-O0	-sALLOW_MEMORY_GROWTH -s "EXPORTED_RUNTIME_METHODS=['ccall']"
fi

if [ "$1" = "raylib" ]; then
	echo "building raylib .."
	mkdir -p lib
	cd src/external/raylib/src
	clang -D_GLFW_X11 -DPLATFORM_DESKTOP -c *.c -I./external/glfw/include
	ar rcs ../../../../lib/libraylib.a *.o
	rm *.o
	cd ../../../..
fi

if [ "$1" = "raylib-web" ]; then
	echo "building raylib for web .."
	mkdir -p lib
	cd src/external/raylib/src 
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rcore.c
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rshapes.c
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rtextures.c
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rtext.c
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rmodels.c
	emcc -DPLATFORM_WEB -Os -c utils.c
	emcc -DPLATFORM_WEB -Os -c raudio.c
	emar rcs ../../../../lib/libraylibweb.a rcore.o rshapes.o rtextures.o rtext.o rmodels.o utils.o raudio.o
	rm *.o
	cd ../../../..
fi

echo "building the app .."
mkdir -p build
clang++ -Wall  -g -ggdb -O3 -std=c++11 -o build/hatori $1 -L./lib -l:libraylib.a -lm -lpthread -lGL -ldl -lrt -lX11 -lglfw
