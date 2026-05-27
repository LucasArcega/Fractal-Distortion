# Script de Instalação de Dependências - Fractal Distortion
# Este script instala todas as dependências necessárias para compilar o projeto

#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Fractal Distortion - Instalador de Dependências" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Função para verificar se um comando existe
function Test-CommandExists {
    param($command)
    $oldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'stop'
    try {
        if (Get-Command $command) { return $true }
    }
    catch { return $false }
    finally { $ErrorActionPreference = $oldPreference }
}

# Função para adicionar ao PATH se necessário
function Add-ToPath {
    param(
        [string]$Path
    )

    if (Test-Path $Path) {
        $currentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
        if ($currentPath -notlike "*$Path*") {
            Write-Host "Adicionando ao PATH: $Path" -ForegroundColor Yellow
            [Environment]::SetEnvironmentVariable(
                "Path",
                "$currentPath;$Path",
                "Machine"
            )
            $env:Path += ";$Path"
            Write-Host "✓ Adicionado ao PATH" -ForegroundColor Green
        } else {
            Write-Host "✓ Já está no PATH" -ForegroundColor Green
        }
    }
}

# 1. Verificar/Instalar Chocolatey
Write-Host "[1/5] Verificando Chocolatey..." -ForegroundColor Yellow
if (Test-CommandExists choco) {
    Write-Host "✓ Chocolatey já instalado" -ForegroundColor Green
} else {
    Write-Host "Instalando Chocolatey..." -ForegroundColor Yellow
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

    if (Test-CommandExists choco) {
        Write-Host "✓ Chocolatey instalado com sucesso" -ForegroundColor Green
    } else {
        Write-Host "✗ Erro ao instalar Chocolatey" -ForegroundColor Red
        Write-Host "Por favor, instale manualmente: https://chocolatey.org/install" -ForegroundColor Red
        exit 1
    }
}
Write-Host ""

# 2. Instalar Git
Write-Host "[2/5] Verificando Git..." -ForegroundColor Yellow
if (Test-CommandExists git) {
    $gitVersion = git --version
    Write-Host "✓ Git já instalado: $gitVersion" -ForegroundColor Green
} else {
    Write-Host "Instalando Git..." -ForegroundColor Yellow
    choco install git -y

    # Atualizar PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    if (Test-CommandExists git) {
        Write-Host "✓ Git instalado com sucesso" -ForegroundColor Green
    } else {
        Write-Host "✗ Erro ao instalar Git. Instale manualmente." -ForegroundColor Red
        Write-Host "Consulte: docs/dependencias.md" -ForegroundColor Yellow
    }
}
Write-Host ""

# 3. Instalar CMake
Write-Host "[3/5] Verificando CMake..." -ForegroundColor Yellow
if (Test-CommandExists cmake) {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Host "✓ CMake já instalado: $cmakeVersion" -ForegroundColor Green

    # Verificar versão mínima (3.22)
    $versionMatch = $cmakeVersion -match "(\d+\.\d+\.\d+)"
    if ($versionMatch) {
        $version = [Version]$matches[1]
        $minVersion = [Version]"3.22.0"

        if ($version -lt $minVersion) {
            Write-Host "⚠ Versão do CMake ($version) é inferior à mínima (3.22)" -ForegroundColor Yellow
            Write-Host "Atualizando CMake..." -ForegroundColor Yellow
            choco upgrade cmake -y
        }
    }
} else {
    Write-Host "Instalando CMake..." -ForegroundColor Yellow
    choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y

    # Atualizar PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    if (Test-CommandExists cmake) {
        Write-Host "✓ CMake instalado com sucesso" -ForegroundColor Green
    } else {
        Write-Host "✗ Erro ao instalar CMake. Instale manualmente." -ForegroundColor Red
        Write-Host "Consulte: docs/dependencias.md" -ForegroundColor Yellow
    }
}
Write-Host ""

# 4. Instalar Ninja
Write-Host "[4/5] Verificando Ninja..." -ForegroundColor Yellow
if (Test-CommandExists ninja) {
    $ninjaVersion = ninja --version
    Write-Host "✓ Ninja já instalado: v$ninjaVersion" -ForegroundColor Green
} else {
    Write-Host "Instalando Ninja..." -ForegroundColor Yellow
    choco install ninja -y

    # Atualizar PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    if (Test-CommandExists ninja) {
        Write-Host "✓ Ninja instalado com sucesso" -ForegroundColor Green
    } else {
        Write-Host "⚠ Ninja não instalado (opcional)" -ForegroundColor Yellow
        Write-Host "Para instalação manual, consulte: docs/dependencias.md" -ForegroundColor Yellow
    }
}
Write-Host ""

# 5. Verificar Visual Studio
Write-Host "[5/5] Verificando Visual Studio 2022..." -ForegroundColor Yellow

$vsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

if (Test-Path $vsWherePath) {
    $vsInstallations = & $vsWherePath -version "[17.0,18.0)" -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

    if ($vsInstallations) {
        Write-Host "✓ Visual Studio 2022 encontrado" -ForegroundColor Green
        Write-Host "  Localização: $vsInstallations" -ForegroundColor Gray

        # Verificar componente C++
        $hasCpp = & $vsWherePath -version "[17.0,18.0)" -products * -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath

        if ($hasCpp) {
            Write-Host "✓ Workload 'Desktop development with C++' instalado" -ForegroundColor Green
        } else {
            Write-Host "⚠ Workload 'Desktop development with C++' NÃO encontrado" -ForegroundColor Yellow
            Write-Host "  Você precisa instalar este componente no Visual Studio Installer" -ForegroundColor Yellow
        }
    } else {
        Write-Host "✗ Visual Studio 2022 não encontrado" -ForegroundColor Red
        Write-Host "  Por favor, instale o Visual Studio 2022 Community Edition" -ForegroundColor Yellow
        Write-Host "  Consulte: docs/dependencias.md para instruções" -ForegroundColor Yellow
    }
} else {
    Write-Host "⚠ Visual Studio Installer não encontrado" -ForegroundColor Yellow
    Write-Host "  Por favor, instale o Visual Studio 2022 Community Edition" -ForegroundColor Yellow
    Write-Host "  Consulte: docs/dependencias.md para instruções" -ForegroundColor Yellow
}
Write-Host ""

# Resumo Final
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Resumo da Instalação" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$allGood = $true

Write-Host "Git........: " -NoNewline
if (Test-CommandExists git) {
    Write-Host "✓ Instalado" -ForegroundColor Green
} else {
    Write-Host "✗ Não instalado" -ForegroundColor Red
    $allGood = $false
}

Write-Host "CMake......: " -NoNewline
if (Test-CommandExists cmake) {
    Write-Host "✓ Instalado" -ForegroundColor Green
} else {
    Write-Host "✗ Não instalado" -ForegroundColor Red
    $allGood = $false
}

Write-Host "Ninja......: " -NoNewline
if (Test-CommandExists ninja) {
    Write-Host "✓ Instalado" -ForegroundColor Green
} else {
    Write-Host "○ Não instalado (opcional)" -ForegroundColor Yellow
}

Write-Host "VS 2022....: " -NoNewline
if (Test-Path $vsWherePath) {
    $vsInstallations = & $vsWherePath -version "[17.0,18.0)" -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsInstallations) {
        Write-Host "✓ Instalado" -ForegroundColor Green
    } else {
        Write-Host "✗ Não encontrado" -ForegroundColor Red
        $allGood = $false
    }
} else {
    Write-Host "✗ Não encontrado" -ForegroundColor Red
    $allGood = $false
}

Write-Host ""

if ($allGood) {
    Write-Host "🎉 Todas as dependências essenciais estão instaladas!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Próximos passos:" -ForegroundColor Cyan
    Write-Host "1. Reinicie o terminal para garantir que o PATH foi atualizado" -ForegroundColor White
    Write-Host "2. Execute: cmake -B build -G `"Visual Studio 17 2022`" -A x64" -ForegroundColor White
    Write-Host "3. Abra a pasta do projeto no Visual Studio 2022" -ForegroundColor White
    Write-Host ""
    Write-Host "Para mais informações, consulte: docs/dependencias.md" -ForegroundColor Gray
} else {
    Write-Host "⚠ Algumas dependências não foram instaladas corretamente" -ForegroundColor Yellow
    Write-Host "Consulte o guia em: docs/dependencias.md" -ForegroundColor Yellow
    Write-Host "Para instalação manual das dependências faltantes" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Pressione qualquer tecla para sair..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
