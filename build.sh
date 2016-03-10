#!/usr/bin/env bash

mkdir -p bin

clang++ -g -I ../assimp/include/ -L ../assimp/lib/ -I ./imgui -lassimp -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -o bin/test build.cpp

