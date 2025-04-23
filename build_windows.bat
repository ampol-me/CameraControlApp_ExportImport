@echo off
setlocal

REM ตรวจสอบว่ามี MinGW-w64 หรือไม่
where gcc >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo MinGW-w64 is not installed. Please install it first.
    echo You can download it from: https://www.mingw-w64.org/
    pause
    exit /b 1
)

REM ตรวจสอบว่ามี CMake หรือไม่
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo CMake is not installed. Please install it first.
    echo You can download it from: https://cmake.org/download/
    pause
    exit /b 1
)

REM สร้างโฟลเดอร์ build
if not exist build mkdir build
cd build

REM รัน CMake
cmake -G "MinGW Makefiles" ..

REM คอมไพล์
mingw32-make

echo.
echo Build completed!
echo The executable should be in the build directory.
pause 