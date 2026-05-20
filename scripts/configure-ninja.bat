@echo off
setlocal
cd /d "%~dp0.."

call "%~dp0setup-vs-ninja.bat"
if errorlevel 1 exit /b 1

call "%~dp0cmake-debug-cache-flags.bat"

if exist "C:\Program Files\CMake\bin\cmake.exe" (
  "C:\Program Files\CMake\bin\cmake.exe" -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug %FRACTAL_DELAY_CMAKE_DEBUG_FLAGS%
) else (
  cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug %FRACTAL_DELAY_CMAKE_DEBUG_FLAGS%
)
exit /b %ERRORLEVEL%
