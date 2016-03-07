#!/usr/bin/env bash

g++ -I ../assimp/include/ -L ../assimp/lib/ -I ./imgui -lassimp -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -o test -O2 build.cpp

