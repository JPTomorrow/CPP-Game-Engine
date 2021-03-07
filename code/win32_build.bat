@echo off

:: First argument is 32 / 64 bit flag
set x86_x64=%1

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" %x86_x64% > NUL

if not exist ".\build" mkdir .\build
pushd .\build

set win32_app_name=win32_application
set win32_entry_point=..\code\win32_platform_layer.cpp
set win32_libs=user32.lib gdi32.lib
set win32_exe=/Fe:%win32_app_name%.exe
set win32_flags=/nologo -MT -Gm- /GR- /EHa- /std:c++17 -Oi -Z7 -FC -Fm%win32_app_name%.map
set win32_warn_flags=-WX -W4 -wd4201 -wd4100 -wd4189
set win32_defines=-DAPPLICATION_INTERNAL=1 -DAPPLICATION_SLOW=1 -DAPPLICATION_WIN32=1
set win32_link=/link -opt:ref 

:: Set windows subsystem for 32-bit
if "%x86_x64%" == "x86" ( set win32_link=%win32_link% -subsystem:windows,5.1)

:: win32 compile
cl %win32_flags% %win32_warn_flags% %win32_defines% %win32_exe% %win32_entry_point% %win32_link% %win32_libs%

popd