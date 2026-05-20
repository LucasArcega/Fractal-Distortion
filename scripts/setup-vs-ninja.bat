@echo off
rem Load MSVC + Windows SDK (required for Ninja from a normal shell).
rem Do not use setlocal/endlocal here — VsDevCmd must persist in the caller's environment.

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSDEV="

if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSDEV=%%i\Common7\Tools\VsDevCmd.bat"

if not defined VSDEV if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set "VSDEV=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" set "VSDEV=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" set "VSDEV=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

if not defined VSDEV (
  echo ERROR: VsDevCmd.bat not found. Install "Desktop development with C++" in Visual Studio Installer.
  exit /b 1
)

call "%VSDEV%" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1

rem Without this, cmake --build from PowerShell finds cl.exe but Catch2/JUCE fail on #include <vector>.
if not defined INCLUDE (
  echo ERROR: INCLUDE is empty after VsDevCmd. MSVC workload "Desktop development with C++" may be missing.
  exit /b 1
)

rem Ninja from winget lives under a versioned folder; prepend any matching package dir.
for /d %%d in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\Ninja-build.Ninja_*") do set "PATH=%%d;%PATH%"

where ninja >nul 2>nul
if errorlevel 1 (
  echo ERROR: ninja.exe not on PATH. Install: winget install Ninja-build.Ninja
  echo Then open a new terminal, or add Ninja's folder to PATH manually.
  exit /b 1
)

exit /b 0
