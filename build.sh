#!/bin/bash

# compile as (hot-reloadable) dll + exe
clang --shared -fPIC -DCOMPILE_DLL -Wall -Wshadow main.c -o code.dll -lGLEW -lGL
clang -DCOMPILE_EXE -Wall -Wshadow main.c -o main -lglfw -lGL -ldl

# compile as standalone executable
#clang -DCOMPILE_EXE -DCOMPILE_DLL -Wall -Wshadow main.c -o main -lglfw -lGLEW -lGL
