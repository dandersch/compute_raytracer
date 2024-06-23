echo >/dev/null # >nul & GOTO WINDOWS & rem ^

# compile as (hot-reloadable) dll + exe
cc --shared -fPIC -DCOMPILE_DLL -Wall -Wshadow main.c -o code.dll -lGLEW -lGL
cc -DCOMPILE_EXE -Wall -Wshadow main.c -o main -lglfw -lGL -ldl -lm

# compile as standalone executable
#cc -DCOMPILE_EXE -DCOMPILE_DLL -Wall -Wshadow main.c -o main -lglfw -lGLEW -lGL -lm

exit 0
:WINDOWS
@echo off

REM note: _CRT_SECURE_NO_WARNINGS because of use of strcpy in tinyobj_loader.c
REM note: M_PI because it is a non-standard define in math.h
clang.exe -I dep/ -DM_PI=3.141 -D_CRT_SECURE_NO_WARNINGS -DCOMPILE_EXE -DCOMPILE_DLL -Wall -Wshadow main.c -o main.exe -Ldep -lglfw3dll -lGLEW -lopengl32 -lgdi32 -Xlinker /NOIMPLIB -Xlinker /NOEXP
REM cl.exe /nologo /I dep/ -DM_PI=3.141 -D_CRT_SECURE_NO_WARNINGS -DCOMPILE_EXE -DCOMPILE_DLL -Wall main.c /link /OUT:main.exe /LIBPATH:dep glfw3dll.lib GLEW.lib opengl32.lib gdi32.lib /NOIMPLIB /NOEXP

REM copy glfw3.dll and glew32.dll into root directory
for %%i in ("dep\*.dll") do ( copy "%%i" "%cd%" )
