@echo off
echo ========================================
echo Reconfigurando Visual Studio para JUCE
echo ========================================
echo.

REM Fechar processos do Visual Studio (se abertos)
echo [1/5] Verificando se VS esta aberto...
tasklist /FI "IMAGENAME eq devenv.exe" 2>NUL | find /I /N "devenv.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo AVISO: Visual Studio esta aberto!
    echo Por favor, FECHE o Visual Studio e pressione qualquer tecla...
    pause >nul
)

REM Limpar cache do Visual Studio (pasta .vs)
echo.
echo [2/5] Limpando cache do Visual Studio...
if exist ".vs" (
    rmdir /s /q ".vs"
    echo Cache .vs removido.
) else (
    echo Sem cache .vs para remover.
)

REM Limpar pasta out antiga (Ninja usava essa)
echo.
echo [3/5] Limpando pasta 'out' antiga...
if exist "out" (
    rmdir /s /q "out"
    echo Pasta 'out' removida.
) else (
    echo Sem pasta 'out' para remover.
)

REM A pasta build ja esta configurada, nao vamos mexer

echo.
echo [4/5] Verificando configuracao CMake...
if exist "build\CMakeCache.txt" (
    echo CMake ja configurado em 'build\'
) else (
    echo ERRO: build\CMakeCache.txt nao encontrado!
    echo Execute primeiro: cmake -B build -G "Visual Studio 17 2022" -A x64
    pause
    exit /b 1
)

echo.
echo [5/5] Tudo pronto!
echo.
echo ========================================
echo PROXIMOS PASSOS:
echo ========================================
echo 1. Abra o Visual Studio 2022
echo 2. Selecione: File ^> Open ^> Folder
echo 3. Escolha esta pasta: %CD%
echo 4. Aguarde o CMake terminar de carregar
echo 5. No topo, selecione: "x64-Debug"
echo 6. Pressione F5 para compilar e executar
echo ========================================
echo.
pause
