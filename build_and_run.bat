@echo off
echo ===================================================
echo Building 3D Sorting Algorithm Visualizer (Raylib C++)
echo ===================================================

set RAYLIB_DIR=C:\raylib-6.0_win64_mingw-w64\raylib-6.0_win64_mingw-w64

g++ main.cpp -O2 -std=c++17 -I"%RAYLIB_DIR%\include" -L"%RAYLIB_DIR%\lib" -lraylib -lopengl32 -lgdi32 -lwinmm -o sorting_visualizer.exe

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation failed!
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [SUCCESS] Compilation successful!
echo Starting 3D Sorting Visualizer...
echo.

start sorting_visualizer.exe
