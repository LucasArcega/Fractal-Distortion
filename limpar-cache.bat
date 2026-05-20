@echo off
echo Limpando cache do CMake e Visual Studio...

REM Deletar pastas de build antigas
if exist "build" rmdir /s /q "build"
if exist "out" rmdir /s /q "out"
if exist ".vs" rmdir /s /q ".vs"

REM Deletar arquivos de cache do CMake
del /f /q CMakeCache.txt 2>nul
del /f /q CMakeSettings.json 2>nul

echo.
echo Cache limpo com sucesso!
echo.
echo Agora:
echo 1. Abra o Visual Studio 2022
echo 2. Selecione "Open a local folder"
echo 3. Escolha esta pasta
echo 4. Aguarde o CMake configurar automaticamente
echo.
pause
