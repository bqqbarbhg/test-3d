@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x64
cd bin 2>NUL || echo Creating bin folder && mkdir bin && cd bin
mkdir data 2>NUL

set IgnoreWarn= -wd4100 -wd4101 -wd4189 -wd4706 -wd4201
set CLFlags= -MDd -DHAS_SSE2 -EHsc -nologo -Od -W4 -WX -Zi %IgnoreWarn% -D_CRT_SECURE_NO_WARNINGS -I "D:\dev\test-3d\imgui" -I D:\include
set LDFlags= -opt:ref -NODEFAULTLIB:MSVCRT -NODEFAULTLIB:libcmt user32.lib gdi32.lib shell32.lib ws2_32.lib DbgHelp.lib opengl32.lib glew32s.lib assimp-vc110-mtd.lib glfw3.lib -LIBPATH:D:\lib

cl %CLFlags% ../build_editor.cpp -DBUILD_DEBUG -link %LDFlags% -out:test.exe
cl %CLFlags% ../build_viewer.cpp -DBUILD_DEBUG -link %LDFlags% -out:viewer.exe

xcopy /eqy ..\data data >NUL
cd ..

