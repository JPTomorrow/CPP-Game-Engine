@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir .\build
pushd .\build
cl -DAPPLICATION_INTERNAL=1 -DAPPLICATION_SLOW=1 -DAPPLICATION_WIN32=1 -Zi -FC /std:c++17 /Fe:win32_application.exe ..\code\win32_platform_layer.cpp user32.lib gdi32.lib
popd