@echo off
REM San Francisco Rush: Extreme Racing - Recompiled
REM Quick build and run script for Windows

setlocal

REM Add LLVM to PATH
set PATH=C:\Program Files\LLVM\bin;%PATH%

REM Configure if needed
if not exist build\build.ninja (
    echo Configuring...
    cmake -S . -B build -G Ninja ^
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
        -DCMAKE_C_COMPILER=clang-cl ^
        -DCMAKE_CXX_COMPILER=clang-cl ^
        -DCMAKE_LINKER=lld-link
    if errorlevel 1 goto :error
)

REM Build
echo Building...
cmake --build build --config RelWithDebInfo
if errorlevel 1 goto :error

REM Run
echo.
echo Running SFRushRecompiled...
echo ==============================
build\SFRushRecompiled.exe
goto :eof

:error
echo Build failed!
pause
