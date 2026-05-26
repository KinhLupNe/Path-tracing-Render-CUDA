@echo off
echo ========================================
echo 1. BUILDING PROJECT (C++ & CUDA)...
echo ========================================
cmake --build build --config Debug

if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================
    echo 2. BUILD SUCCESSFUL! LAUNCHING APP...
    echo ========================================
    build\Debug\ScratchPathTracerCUDA.exe
) else (
    echo.
    echo ========================================
    echo [X] BUILD FAILED! PLEASE CHECK ERRORS.
    echo ========================================
)
