@echo off
setlocal enabledelayedexpansion

set VSTOOLS="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
call %VSTOOLS%

if not exist ".\build" mkdir .\build
if not exist ".\bin" mkdir .\bin

set FLAGS=/Fe: ./bin/emulator.exe /Fo"build\\" /Fd"build\\" /std:c++latest /EHsc /FC /Zi
set LIBS=user32.lib gdi32.lib
FOR /F "tokens=*" %%g in ('dir /s/b .\src\*.cpp') do (set "CPP=!CPP! %%g")

IF "%1"=="/t" (
    set FLAGS=/Fe: ./bin/test.exe /Fo"build\\" /Fd"build\\" /std:c++latest /EHsc /FC /Zi /I%~dp0src /I%~dp0test
    set CPP=test/main.cpp test/memory_bus.cpp src/cpu.cpp src/joypad.cpp src/ppu.cpp src/timers.cpp src/emulator.cpp src/win32.cpp
)

cl %CPP% %LIBS% %FLAGS%



