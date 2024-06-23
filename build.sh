#!/bin/bash

# compile as (hot-reloadable) dll + exe
#cc --shared -fPIC -DCOMPILE_DLL -Wall -Wshadow main.c -o code.dll -lGLEW -lGL
#cc -DCOMPILE_EXE -Wall -Wshadow main.c -o main -lglfw -lGL -ldl -lm

# compile as standalone executable
cc -DCOMPILE_EXE -DCOMPILE_DLL -Wall -Wshadow main.c -o main -lglfw -lGLEW -lGL -lm
