#!/bin/bash

if [ "$1" = "build" ]; then
	echo "building the app .."
	mkdir -p build
	clang++ -o build/main ./main.cpp -L./lib -l:libraylib.a -lm -lpthread -lGL  -ldl -lrt -lX11 -lglfw
fi

if [ "$1" = "build-web" ]; then
	echo "building the app for web"
	mkdir -p build
	emcc -o build/index.html ./main.cpp -I./raylib/src -L./lib -l:libraylibweb.a \
	-s USE_GLFW=3 -s ASYNCIFY --shell-file=hatori.html -s EXPORT_ALL=1
fi

if [ "$1" = "raylib" ]; then
	echo "building raylib .."
	mkdir -p lib
	cd raylib/src
	clang -D_GLFW_X11 -DPLATFORM_DESKTOP -c *.c -I./external/glfw/include
	ar rcs ../../lib/libraylib.a *.o
	rm *.o
	cd ../..
fi

if [ "$1" = "raylib-web" ]; then
	echo "building raylib for web .."
	mkdir -p lib
	cd raylib/src 
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rcore.c
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rshapes.c
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rtextures.c
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rtext.c
	emcc -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB -Os -c rmodels.c
	emcc -DPLATFORM_WEB -Os -c utils.c
	emcc -DPLATFORM_WEB -Os -c raudio.c
	emar rcs ../../lib/libraylibweb.a rcore.o rshapes.o rtextures.o rtext.o rmodels.o utils.o raudio.o
	rm *.o
	cd ../..
fi