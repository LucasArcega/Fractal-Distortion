@echo off
rem Shared CMake cache flags for Debug + Ninja (all-debug, configure-ninja).
rem Do not use setlocal — callers must inherit FRACTAL_DELAY_CMAKE_DEBUG_FLAGS.
rem
rem [gui] tests excluded from ctest on Windows: Catch2 has no JUCE message pump,
rem so the InternalMessageQueue accumulates without dispatch and the process OOMs.
rem The CI runs the same tests on Linux via xvfb-run which provides a proper pump.
rem To run [gui] tests manually: FractalDelay_Tests.exe  (no filter argument)
set "FRACTAL_DELAY_CMAKE_DEBUG_FLAGS=-DFRACTAL_DELAY_CTEST_INCLUDE_GUI=OFF"
