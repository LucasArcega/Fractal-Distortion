# Guia de Dependências - Fractal Distortion

Este documento descreve todas as dependências necessárias para compilar e desenvolver o projeto Fractal Distortion.

## Índice

- [Instalação Automática](#instalação-automática)
- [Dependências Obrigatórias](#dependências-obrigatórias)
- [Dependências Opcionais](#dependências-opcionais)
- [Instalação Manual](#instalação-manual)
- [Verificação da Instalação](#verificação-da-instalação)
- [Solução de Problemas](#solução-de-problemas)

---

## Instalação Automática

O projeto inclui um script PowerShell que instala todas as dependências automaticamente:

### Windows (PowerShell - Requer Admin)

```powershell
# Execute como Administrador
.\install-dependencies.ps1
```

**Nota**: Você precisará executar o PowerShell como Administrador (botão direito > "Executar como Administrador").

Se o script falhar em alguma etapa, consulte a seção [Instalação Manual](#instalação-manual) abaixo.

---

## Dependências Obrigatórias

### 1. Git

**Versão**: 2.30 ou superior

**Para que serve**: Gerenciamento de código fonte e download automático de bibliotecas (JUCE, Catch2).

**Verificar instalação**:
```bash
git --version
```

**Como baixar manualmente**:
1. Acesse: [https://git-scm.com/downloads](https://git-scm.com/downloads)
2. Baixe o instalador para Windows (64-bit)
3. Execute o instalador
4. Durante a instalação:
   - Marque "Git from the command line and also from 3rd-party software"
   - Marque "Use Windows' default console window"
5. Reinicie o terminal após a instalação

**Chocolatey**:
```powershell
choco install git -y
```

---

### 2. CMake

**Versão**: 3.22 ou superior

**Para que serve**: Sistema de build que gera os arquivos de projeto para Visual Studio.

**Verificar instalação**:
```bash
cmake --version
```

**Como baixar manualmente**:
1. Acesse: [https://cmake.org/download/](https://cmake.org/download/)
2. Baixe o instalador Windows x64 (arquivo .msi)
3. Execute o instalador
4. **IMPORTANTE**: Marque "Add CMake to the system PATH for all users"
5. Complete a instalação
6. Reinicie o terminal

**Chocolatey**:
```powershell
choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y
```

---

### 3. Visual Studio 2022

**Versão**: Visual Studio 2022 Community (ou superior)

**Para que serve**: Compilador C++ (MSVC) e ambiente de desenvolvimento.

**Verificar instalação**:
```powershell
# No PowerShell
"${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -version "[17.0,18.0)"
```

**Como baixar manualmente**:

1. **Download**:
   - Acesse: [https://visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
   - Baixe "Visual Studio 2022 Community" (gratuito)

2. **Instalação**:
   - Execute o instalador
   - Na tela "Workloads", selecione:
     - ✅ **Desktop development with C++**

3. **Componentes Individuais** (aba "Individual components"):
   - ✅ `MSVC v143 - VS 2022 C++ x64/x86 build tools (Latest)`
   - ✅ `Windows 10 SDK` ou `Windows 11 SDK` (a versão mais recente)
   - ✅ `C++ CMake tools for Windows`
   - ✅ `C++ core features`

4. **Instalação**:
   - Clique em "Install"
   - Aguarde a conclusão (pode levar 30-60 minutos)

**Chocolatey** (apenas para instalar o instalador, você ainda precisará configurar workloads):
```powershell
choco install visualstudio2022community -y
```

---

### 4. C++ Build Tools (incluído no Visual Studio)

**Componentes necessários**:
- MSVC v143 (Microsoft Visual C++ 2022)
- Windows SDK (10 ou 11)
- CMake tools for Windows

Estes componentes são instalados junto com o workload "Desktop development with C++" do Visual Studio.

**Verificar se o compilador está acessível**:
```cmd
# Abra "x64 Native Tools Command Prompt for VS 2022"
cl
```

Se o comando `cl` retornar informações sobre o compilador, está configurado corretamente.

---

## Dependências Opcionais

### Ninja

**Versão**: 1.10 ou superior

**Para que serve**: Build system mais rápido que o MSBuild (padrão do Visual Studio). **Opcional mas recomendado**.

**Verificar instalação**:
```bash
ninja --version
```

**Como baixar manualmente**:

**Opção 1 - Chocolatey (Recomendado)**:
```powershell
choco install ninja -y
```

**Opção 2 - Download direto**:
1. Acesse: [https://github.com/ninja-build/ninja/releases](https://github.com/ninja-build/ninja/releases)
2. Baixe `ninja-win.zip`
3. Extraia o arquivo `ninja.exe` para uma pasta (ex: `C:\Tools\ninja\`)
4. Adicione a pasta ao PATH:
   - Abra "Editar as variáveis de ambiente do sistema"
   - Clique em "Variáveis de Ambiente"
   - Em "Variáveis do sistema", selecione "Path"
   - Clique em "Editar"
   - Clique em "Novo"
   - Adicione: `C:\Tools\ninja`
   - Clique em "OK" em todas as janelas
5. Reinicie o terminal

---

## Dependências Automaticamente Gerenciadas

As seguintes bibliotecas são baixadas automaticamente pelo CMake via `FetchContent`:

### JUCE Framework
- **Versão**: 8.0.12
- **Repositório**: [https://github.com/juce-framework/JUCE](https://github.com/juce-framework/JUCE)
- **Gerenciado por**: CMake FetchContent
- **Não requer instalação manual**

### Catch2
- **Versão**: 3.5.4
- **Repositório**: [https://github.com/catchorg/Catch2](https://github.com/catchorg/Catch2)
- **Gerenciado por**: CMake FetchContent (apenas para testes)
- **Não requer instalação manual**

---

## Verificação da Instalação

Após instalar as dependências, execute estes comandos para verificar:

```bash
# Verificar Git
git --version
# Esperado: git version 2.x.x

# Verificar CMake
cmake --version
# Esperado: cmake version 3.22.0 ou superior

# Verificar Ninja (opcional)
ninja --version
# Esperado: 1.10.0 ou superior

# Verificar Visual Studio
"${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -version "[17.0,18.0)"
# Deve mostrar o caminho de instalação
```

### Teste Completo: Configurar o Projeto

```bash
# Limpar cache anterior (se existir)
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

# Gerar projeto Visual Studio
cmake -B build -G "Visual Studio 17 2022" -A x64

# Ou usar Ninja (mais rápido)
# Abra "x64 Native Tools Command Prompt for VS 2022" e execute:
# cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

Se o comando acima executar sem erros, as dependências estão instaladas corretamente.

---

## Configuração do PATH

Algumas ferramentas precisam estar no `PATH` do sistema para funcionar corretamente:

### Verificar se está no PATH

```powershell
# Verificar PATH atual
$env:Path -split ';'
```

### Adicionar ao PATH Manualmente

1. Pressione `Win + X` e selecione "Sistema"
2. Clique em "Configurações avançadas do sistema"
3. Clique em "Variáveis de Ambiente"
4. Em "Variáveis do sistema", selecione "Path"
5. Clique em "Editar"
6. Clique em "Novo" e adicione os caminhos necessários:

**Caminhos comuns**:
- Git: `C:\Program Files\Git\cmd`
- CMake: `C:\Program Files\CMake\bin`
- Ninja: `C:\Tools\ninja` (ou onde você extraiu)

7. Clique em "OK" em todas as janelas
8. **Reinicie o terminal** para aplicar as mudanças

---

## Solução de Problemas

### Problema 1: "cmake not found" após instalar

**Causa**: PATH não foi atualizado ou CMake não foi adicionado ao PATH durante instalação.

**Solução**:
1. Reinstale o CMake e marque "Add CMake to the system PATH"
2. Ou adicione manualmente ao PATH (veja seção acima)
3. Reinicie o terminal

---

### Problema 2: "Git not found" ao executar CMake

**Causa**: Git não está no PATH ou não foi instalado.

**Solução**:
1. Verifique se Git está instalado: `git --version`
2. Se não, instale o Git (veja seção acima)
3. Adicione ao PATH se necessário
4. Reinicie o terminal

---

### Problema 3: "MSVC cannot see C++ standard library headers"

**Causa**: CMake foi executado fora do ambiente do Visual Studio.

**Solução**:

**Opção A - Usar Visual Studio Generator**:
```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
```

**Opção B - Usar "x64 Native Tools Command Prompt for VS 2022"**:
1. Abra o menu Iniciar
2. Procure por "x64 Native Tools Command Prompt for VS 2022"
3. Execute os comandos CMake a partir deste terminal

**Opção C - Executar vcvars64.bat antes de usar Ninja**:
```cmd
# Encontre vcvars64.bat (geralmente em):
# C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat

"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Depois execute CMake com Ninja
cmake -B build -G Ninja
```

---

### Problema 4: Visual Studio não reconhece includes do JUCE

**Causa**: IntelliSense não encontrou os headers gerados pelo CMake.

**Solução**:
1. Certifique-se de que CMake foi executado com sucesso
2. No Visual Studio, vá em `Project > Delete Cache and Reconfigure`
3. Aguarde o CMake terminar
4. Se ainda não funcionar, consulte: `RESOLVER-JUCE-VS.md`

---

### Problema 5: "Ninja not found" ao tentar build

**Causa**: Ninja não está instalado ou não está no PATH.

**Solução**:
1. Instale o Ninja (veja seção acima)
2. Ou use o generator do Visual Studio em vez de Ninja:
   ```bash
   cmake -B build -G "Visual Studio 17 2022" -A x64
   ```

---

### Problema 6: Chocolatey não é reconhecido

**Causa**: Chocolatey não está instalado.

**Solução**:
```powershell
# Execute como Administrador
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

Reinicie o terminal e tente novamente.

---

## Requisitos de Sistema

### Sistema Operacional
- Windows 10 (versão 1909 ou superior)
- Windows 11

### Hardware
- **CPU**: x64 (64-bit)
- **RAM**: Mínimo 8 GB (16 GB recomendado para compilação mais rápida)
- **Espaço em Disco**:
  - Visual Studio 2022: ~10-15 GB
  - Projeto + dependências: ~2-3 GB
  - Total recomendado: 20 GB livres

### Conexão com Internet
Necessária para:
- Download inicial das dependências
- CMake FetchContent baixar JUCE e Catch2
- Atualizações do Visual Studio

---

## Referências

- [JUCE Documentation](https://juce.com/learn/documentation)
- [CMake Documentation](https://cmake.org/documentation/)
- [Visual Studio C++ Documentation](https://docs.microsoft.com/en-us/cpp/)
- [Ninja Build System](https://ninja-build.org/)
- [Chocolatey Package Manager](https://chocolatey.org/)

---

## Próximos Passos

Após instalar todas as dependências:

1. **Configure o projeto**:
   ```bash
   cmake -B build -G "Visual Studio 17 2022" -A x64
   ```

2. **Abra no Visual Studio**:
   - File > Open > Folder
   - Selecione a pasta do projeto

3. **Compile**:
   - Pressione `Ctrl+Shift+B`
   - Ou clique em "Build > Build All"

4. **Execute**:
   - Pressione `F5` para rodar com debugger
   - Ou `Ctrl+F5` para rodar sem debugger

Para mais informações sobre o desenvolvimento, consulte:
- `RESOLVER-JUCE-VS.md` - Solução de problemas do Visual Studio
- `GUIA-FRACTAL-DISTORTION-INCREMENTAL.md` - Guia de desenvolvimento do plugin

---

**Última atualização**: 2026-05-23
**Versões testadas**:
- Git 2.45+
- CMake 3.31+
- Visual Studio 2022 (17.0+)
- Ninja 1.11+
- JUCE 8.0.12
