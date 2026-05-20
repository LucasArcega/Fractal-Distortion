@echo off
rem One shot: MSVC env + Ninja + CMake configure (Debug) + full build + ctest.
setlocal
cd /d "%~dp0.."

call "%~dp0setup-vs-ninja.bat"
if errorlevel 1 exit /b 1

set "CMAKE=cmake"
if exist "C:\Program Files\CMake\bin\cmake.exe" set "CMAKE=C:\Program Files\CMake\bin\cmake.exe"

call "%~dp0cmake-debug-cache-flags.bat"

echo.
echo === [1/3] CMake configure (Ninja, Debug^) ===
"%CMAKE%" -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug %FRACTAL_DELAY_CMAKE_DEBUG_FLAGS%
if errorlevel 1 exit /b 1

echo.
echo === [2/3] Build ===
"%CMAKE%" --build build
if errorlevel 1 exit /b 1

echo.
echo === [3/3] ctest ===
ctest --test-dir build --output-on-failure
exit /b %ERRORLEVEL%
