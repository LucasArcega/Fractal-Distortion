# Fractal Distortion — Guia Incremental com Feedback Rápido

## Princípio fundamental deste guia

```text
Ciclo de aprendizado eficaz:

1. CRIAR código mínimo funcional
2. COMPILAR e ver na tela
3. ENTENDER por que funciona
4. OTIMIZAR (se necessário)
5. REPETIR com próxima feature
```

**Regra de ouro:** Você nunca passará mais de **10 minutos** sem compilar e ver algo visual ou audível.

---

## Índice por Fases

Cada fase é **autocontida** e **testável**:

- [Fase 0: Estrutura base (já existe)](#fase-0-estrutura-base-já-existe)
- [Fase 1: Primeiro knob visível (5 min)](#fase-1-primeiro-knob-visível-5-min)
- [Fase 2: Knob controla volume (5 min)](#fase-2-knob-controla-volume-5-min)
- [Fase 3: Volume vira saturação tanh (3 min)](#fase-3-volume-vira-saturação-tanh-3-min)
- [Fase 4: Estilizar o knob (5 min)](#fase-4-estilizar-o-knob-5-min)
- [Fase 5: TubeDistortion real (15 min)](#fase-5-tubedistortion-real-15-min)
- [Fase 6: ComboBox para escolher modos (10 min)](#fase-6-combobox-para-escolher-modos-10-min)
- [Fase 7: DistortionEngine com múltiplos algoritmos (10 min)](#fase-7-distortionengine-com-múltiplos-algoritmos-10-min)
- [Fase 8: Adicionar Bias e Tone (10 min)](#fase-8-adicionar-bias-e-tone-10-min)
- [Fase 9: Otimizações e polimento](#fase-9-otimizações-e-polimento)

---

# Fase 0: Estrutura base (já existe)

Você já tem:

```text
FractalDistortion/
├── CMakeLists.txt
└── Source/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    └── PluginEditor.cpp
```

**Teste:** Compile o projeto. O plugin abre uma janela vazia.

```bash
# Windows (MSVC + Ninja):
scripts\configure-ninja.bat
cmake --build build --target FractalDistortion_Standalone

# Ou abra no Visual Studio e compile
```

**Status:** ✅ Plugin compila e abre janela vazia.

---

# Fase 1: Primeiro knob visível (5 min)

**Objetivo:** Ver um slider/knob na tela. Sem áudio ainda.

## 1.1 Criar `ParameterIDs.h`

**Por quê?** Centralizar IDs evita strings mágicas espalhadas.

**Arquivo:** `Source/ParameterIDs.h`

```cpp
#pragma once
#include <JuceHeader.h>

namespace ParameterIDs
{
    inline constexpr auto driveDb = juce::ParameterID { "driveDb", 1 };
}
```

**Explicação:**
- `inline constexpr`: define constante em header sem erro de múltipla definição
- `"driveDb"`: ID salvo em presets/sessões (nunca mude depois de publicar)
- `1`: versão do parâmetro (para migração futura)

---

## 1.2 Registrar parâmetro no APVTS

**Por quê?** Sem isso, o attachment vai travar o plugin.

**Arquivo:** `Source/PluginProcessor.cpp`

Procure a função `createParameterLayout()`. Se não existir, crie:

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout
FractalDistortionAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::driveDb,           // ID centralizado
        "Drive",                          // Nome visível para usuário/DAW
        juce::NormalisableRange<float>(0.0f, 36.0f, 0.1f, 0.35f),  // min, max, step, skew
        6.0f                              // valor padrão
    ));

    return layout;
}
```

**Explicação linha por linha:**

| Linha | O que faz | Por quê |
|-------|-----------|---------|
| `ParameterIDs::driveDb` | Usa ID centralizado | Evita typo; fácil refatorar |
| `"Drive"` | Nome no host/DAW | Usuário vê isso nos dropdowns |
| `0.0f, 36.0f` | Range em dB | 0 dB = sem ganho; 36 dB = ~63x |
| `0.1f` | Precisão | Slider muda em passos de 0.1 dB |
| `0.35f` | Skew factor | Mais precisão em valores baixos/médios |
| `6.0f` | Padrão | Plugin abre com Drive = 6 dB |

**Por quê skew 0.35?** Sem skew, o controle é linear. Com skew < 1.0, você ganha mais resolução nos valores baixos onde o ouvido é mais sensível. Exemplo:

```text
Slider a 25% da posição:
- Sem skew: 9 dB    (25% de 36 dB)
- Com 0.35: ~3 dB   (mais controle fino na faixa útil)
```

**No construtor do Processor**, inicialize o APVTS:

```cpp
FractalDistortionAudioProcessor::FractalDistortionAudioProcessor()
    : AudioProcessor(BusesProperties()
                      .withInput("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}
```

---

## 1.3 Criar painel com slider simples

**Por quê?** Validar o fluxo UI → APVTS antes de estilizar.

**Arquivo:** `Source/Components/Panels/DistortionPanel/DistortionPanel.h`

```cpp
#pragma once
#include <JuceHeader.h>
#include "ParameterIDs.h"

class DistortionPanel : public juce::Component
{
public:
    explicit DistortionPanel(juce::AudioProcessorValueTreeState& state)
    {
        // Configurar slider visual
        driveSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        driveSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
        addAndMakeVisible(driveSlider);

        // Conectar ao parâmetro
        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state,
            ParameterIDs::driveDb.paramID,
            driveSlider
        );
    }

    void resized() override
    {
        driveSlider.setBounds(getLocalBounds().reduced(12));
    }

private:
    juce::Slider driveSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
};
```

**Explicação:**

```cpp
driveSlider.setSliderStyle(juce::Slider::LinearHorizontal);
```
→ Slider horizontal (fácil de ver para testar).

```cpp
addAndMakeVisible(driveSlider);
```
→ **CRÍTICO**: Sem isso, o slider existe mas não aparece.

```cpp
driveAttachment = std::make_unique<...>(...);
```
→ **CRÍTICO**: Attachment precisa ser **membro da classe**. Se for variável local, morre no fim do construtor.

```cpp
getLocalBounds().reduced(12)
```
→ Equivalente a `padding: 12px` no CSS.

---

## 1.4 Adicionar painel ao Editor

**Arquivo:** `Source/PluginEditor.h`

Adicione o include e o membro:

```cpp
#include "Components/Panels/DistortionPanel/DistortionPanel.h"

class FractalDistortionAudioProcessorEditor : public juce::AudioProcessorEditor
{
    // ... outros membros ...

private:
    DistortionPanel distortionPanel;  // Adicione isto
};
```

**Arquivo:** `Source/PluginEditor.cpp`

No construtor, inicialize no member initializer list:

```cpp
FractalDistortionAudioProcessorEditor::FractalDistortionAudioProcessorEditor(
    FractalDistortionAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      distortionPanel(audioProcessor.getAPVTS())  // Inicialize aqui
{
    setSize(400, 300);
    addAndMakeVisible(distortionPanel);
}
```

No `resized()`:

```cpp
void FractalDistortionAudioProcessorEditor::resized()
{
    distortionPanel.setBounds(getLocalBounds().reduced(20));
}
```

---

## ✅ Checkpoint Fase 1: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve ver:**
- Janela do plugin abre
- Slider horizontal visível
- Arrastar o slider muda o número na caixa de texto
- Fechar e reabrir: valor persiste

**Se quebrar:**

| Erro | Causa provável | Solução |
|------|----------------|---------|
| Travou ao abrir | ID não registrado no APVTS | Verifique `createParameterLayout()` |
| Slider não aparece | Faltou `addAndMakeVisible` | Adicione no construtor |
| Valor não muda | Attachment errado | Verifique `getParamID()` |

---

# Fase 2: Knob controla volume (5 min)

**Objetivo:** Slider mexe no áudio (mesmo que seja só volume simples).

## 2.1 Processar áudio no Processor

**Arquivo:** `Source/PluginProcessor.cpp`

Na função `processBlock()`:

```cpp
void FractalDistortionAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // 1. Ler parâmetro
    const float driveDb = apvts.getRawParameterValue(ParameterIDs::driveDb.paramID)->load();

    // 2. Converter dB → ganho linear
    const float driveGain = juce::Decibels::decibelsToGain(driveDb);

    // 3. Aplicar ganho em todas as amostras
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            samples[sample] *= driveGain;
        }
    }
}
```

**Explicação:**

```cpp
apvts.getRawParameterValue(...)->load()
```
- `getRawParameterValue`: retorna `std::atomic<float>*`
- `load()`: lê valor thread-safe (áudio lê; UI escreve)

```cpp
juce::Decibels::decibelsToGain(driveDb)
```
- Converte: `0 dB → 1.0`, `6 dB → 2.0`, `12 dB → 4.0`
- Fórmula: `10^(dB/20)`

```cpp
samples[sample] *= driveGain;
```
- Multiplica amostra pelo ganho
- Ainda não distorce; é só volume

---

## ✅ Checkpoint Fase 2: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve:**
1. Abrir o Standalone
2. Gerar tom (usar gerador de tom da DAW ou tocar áudio)
3. Mexer no slider Drive
4. **Ouvir** volume mudar

**Se não ouvir nada:**
- Verifique se o áudio está passando (medidor visual?)
- Coloque `DBG(driveDb)` no `processBlock()` e veja console

---

# Fase 3: Volume vira saturação tanh (3 min)

**Objetivo:** Fazer o Drive distorcer, não só amplificar.

**Arquivo:** `Source/PluginProcessor.cpp`

Mude apenas a linha de processamento:

```cpp
// ANTES (Fase 2):
samples[sample] *= driveGain;

// AGORA (Fase 3):
samples[sample] = std::tanh(samples[sample] * driveGain);
```

**Por quê `std::tanh`?**

| Função | Entrada → Saída | Quando usar |
|--------|-----------------|-------------|
| `x * gain` | `-1 → -gain`, `+1 → +gain` | Volume puro (linear) |
| `std::tanh(x * gain)` | `±∞ → ±1` | Saturação suave (válvula) |
| `juce::jlimit(-1, 1, x * gain)` | Corta em ±1 | Hard clip (agressivo) |

**Como `tanh` funciona:**

```text
Input (após ganho) → tanh → Output

   0.0  →  0.0  (silêncio não muda)
   0.5  →  0.46 (baixo ganho: quase linear)
   1.0  →  0.76 (começa a comprimir)
   2.0  →  0.96 (comprime muito)
   5.0  →  0.9999 (satura em 1.0)
```

---

## ✅ Checkpoint Fase 3: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve:**
1. Tocar áudio com Drive = 0 dB → som limpo
2. Aumentar Drive → som distorce, mas não explode (não clippa hard)
3. Drive = 36 dB → saturação pesada

**Comparação:**
- **Fase 2** (volume): Drive alto explode/clippa
- **Fase 3** (tanh): Drive alto satura suave

---

# Fase 4: Estilizar o knob (5 min)

**Objetivo:** Trocar slider horizontal por knob rotativo estilizado.

**Por quê fazer agora?**
- Funcionalidade validada ✅
- Knob rotativo é padrão da indústria
- `StyleUtils.h` já existe no projeto

---

## 4.1 Trocar para knob rotativo

**Arquivo:** `Source/Components/Panels/DistortionPanel/DistortionPanel.h`

Substitua o conteúdo por:

```cpp
#pragma once
#include <JuceHeader.h>
#include "ParameterIDs.h"
#include "Components/LabeledSlider/LabeledSlider.h"

class DistortionPanel : public juce::Component
{
public:
    explicit DistortionPanel(juce::AudioProcessorValueTreeState& state)
        : driveControl("Drive")  // Label "Drive" acima do knob
    {
        addAndMakeVisible(driveControl);

        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state,
            ParameterIDs::driveDb.paramID,
            driveControl.getSlider()  // Pega referência do slider interno
        );
    }

    void resized() override
    {
        driveControl.setBounds(getLocalBounds().reduced(12));
    }

private:
    Common::LabeledSlider driveControl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
};
```

**Mudanças:**

| Antes | Agora | Por quê |
|-------|-------|---------|
| `juce::Slider driveSlider` | `Common::LabeledSlider driveControl` | Knob + label em um componente |
| `driveSlider` direto | `driveControl.getSlider()` | Attachment no slider interno |

---

## 4.2 Entender o LabeledSlider

**Arquivo:** `Source/Components/LabeledSlider/LabeledSlider.h` (já existe)

```cpp
namespace Common {
    class LabeledSlider : public juce::Component {
    public:
        explicit LabeledSlider(const juce::String& labelText,
                               juce::uint32 accentColour = GUI::Colors::AccentOrange)
        {
            // 1. Estiliza label
            GUI::styleParameterLabel(label, labelText, ...);
            addAndMakeVisible(label);

            // 2. Estiliza knob rotativo
            GUI::styleRotaryKnob(slider, accentColour, true);
            addAndMakeVisible(slider);
        }

        juce::Slider& getSlider() noexcept { return slider; }

        void resized() override
        {
            // Layout vertical: Label (14px) + Knob (resto, mínimo 72px)
            juce::FlexBox layout;
            layout.flexDirection = juce::FlexBox::Direction::column;
            layout.items.add(juce::FlexItem(label).withHeight(14.f));
            layout.items.add(juce::FlexItem(slider).withFlex(1.f).withMinHeight(72.f));
            layout.performLayout(getLocalBounds().toFloat());
        }

    private:
        juce::Label label;
        juce::Slider slider;
    };
}
```

**Por quê criar `LabeledSlider`?**
- Evita repetir "label + knob" em cada painel
- Estilo consistente (todas as cores, ângulos, fontes centralizados)
- Fácil manutenção: mudar `StyleUtils` muda todos os knobs

---

## ✅ Checkpoint Fase 4: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve ver:**
- Knob rotativo (arco preenchido laranja)
- Label "Drive" acima do knob
- Caixa de texto abaixo com valor em dB
- Arrastar verticalmente muda valor
- **Áudio continua funcionando igual Fase 3**

---

# Fase 5: TubeDistortion real (15 min)

**Objetivo:** Substituir `std::tanh` simples por algoritmo Tube com filtros e bias.

**Por quê?**
- `tanh` sozinho é saturação genérica
- Tube real tem: high-pass (remover DC), bias (assimetria), tone (dark/bright)

---

## 5.1 Criar classe TubeDistortion

**Arquivo:** `Source/DSP/TubeDistortion.h` (criar diretório `DSP` se não existir)

```cpp
#pragma once
#include <JuceHeader.h>

namespace DSP
{
class TubeDistortion
{
public:
    void prepare(double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        reset();
        updateFilters();
    }

    void reset() noexcept
    {
        hpState = 0.0f;
        hpLastInput = 0.0f;
        lpState = 0.0f;
    }

    void setDriveDb(float db) noexcept
    {
        driveGain = juce::Decibels::decibelsToGain(db);
    }

    void setBias(float newBias) noexcept
    {
        bias = juce::jlimit(-0.5f, 0.5f, newBias);
    }

    void setToneHz(float hz) noexcept
    {
        toneHz = juce::jlimit(1000.0f, 20000.0f, hz);
        updateFilters();
    }

    float processSample(float input) noexcept
    {
        // Pipeline Tube clássico:
        const float filteredInput = highPass(input);      // Remove DC
        const float biasedInput = filteredInput + bias;   // Adiciona assimetria
        const float drivenInput = biasedInput * driveGain;// Amplifica
        const float saturated = std::tanh(drivenInput);   // Satura
        const float dcOffset = std::tanh(bias * driveGain); // Correção DC do bias
        const float corrected = saturated - dcOffset;     // Remove DC gerado
        const float toned = lowPass(corrected);           // Suaviza agudos

        return toned;
    }

private:
    void updateFilters() noexcept
    {
        // High-pass a 25 Hz (remove DC)
        const float hpHz = 25.0f;
        hpAlpha = std::exp(-juce::MathConstants<float>::twoPi * hpHz / static_cast<float>(sampleRate));

        // Low-pass (Tone)
        lpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * toneHz / static_cast<float>(sampleRate));
    }

    float highPass(float input) noexcept
    {
        const float output = hpAlpha * (hpState + input - hpLastInput);
        hpState = output;
        hpLastInput = input;
        return output;
    }

    float lowPass(float input) noexcept
    {
        lpState += lpCoeff * (input - lpState);
        return lpState;
    }

    double sampleRate = 44100.0;

    float driveGain = 1.0f;
    float bias = 0.0f;
    float toneHz = 16000.0f;

    float hpAlpha = 0.0f;
    float hpState = 0.0f;
    float hpLastInput = 0.0f;

    float lpCoeff = 1.0f;
    float lpState = 0.0f;
};
} // namespace DSP
```

**Explicação das partes:**

### `prepare()` — Inicialização
```cpp
void prepare(double newSampleRate)
```
→ Chamado pelo Processor quando sample rate muda (44.1k, 48k, 96k, etc.).

### `reset()` — Limpar estado
```cpp
hpState = 0.0f;
lpState = 0.0f;
```
→ Zera memória dos filtros (evita clicks ao mudar preset/bypass).

### `processSample()` — Pipeline explicado

```text
Input
  ↓
highPass(25 Hz) ← Remove DC do input
  ↓
+ bias          ← Desloca curva (assimetria)
  ↓
× driveGain     ← Amplifica
  ↓
tanh()          ← Satura
  ↓
- dcOffset      ← Corrige DC gerado pelo bias
  ↓
lowPass(Tone)   ← Escurece (corta agudos estridentes)
  ↓
Output
```

**Por quê cada etapa?**

| Etapa | Sem ela | Com ela |
|-------|---------|---------|
| `highPass` | DC no input causa pops | Remove DC |
| `bias` | Simétrico (frio) | Assimétrico (quente, válvula) |
| `dcOffset` | Bias gera DC na saída | Corrige DC |
| `lowPass` | Agudos estridentes | Som escuro/quente |

---

## 5.2 Usar TubeDistortion no Processor

**Arquivo:** `Source/PluginProcessor.h`

Adicione o include e membro:

```cpp
#include "DSP/TubeDistortion.h"

class FractalDistortionAudioProcessor : public juce::AudioProcessor
{
    // ... outros membros ...

private:
    std::vector<DSP::TubeDistortion> tubeEngines;  // Uma por canal
};
```

**Arquivo:** `Source/PluginProcessor.cpp`

Na função `prepareToPlay()`:

```cpp
void FractalDistortionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Criar uma engine por canal (stereo = 2)
    tubeEngines.clear();
    tubeEngines.resize(getTotalNumOutputChannels());

    for (auto& tube : tubeEngines)
    {
        tube.prepare(sampleRate);
        tube.setToneHz(16000.0f);  // Padrão: bright
    }
}
```

**Por quê uma engine por canal?**
- Filtros tem **estado** (memória do sample anterior)
- Se compartilhar: canal esquerdo "vaza" pro direito
- Stereo precisa de 2 engines independentes

---

Na função `processBlock()`:

```cpp
void FractalDistortionAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float driveDb = apvts.getRawParameterValue(ParameterIDs::driveDb.paramID)->load();

    // Configurar todas as engines com mesmo Drive
    for (auto& tube : tubeEngines)
        tube.setDriveDb(driveDb);

    // Processar cada canal
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        auto& tube = tubeEngines[channel];

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            samples[sample] = tube.processSample(samples[sample]);
        }
    }
}
```

**⚠️ Nota sobre performance:**

Este código funciona, mas chama `setDriveDb()` a cada bloco (~20 mil vezes por segundo), mesmo que o valor não tenha mudado. Isso desperdiça CPU em conversões `decibelsToGain()` repetidas.

**Por quê não otimizar agora?**
- Prioridade: ver funcionar primeiro
- A otimização é explicada na **Fase 9** com comparação antes/depois
- CPU moderna aguenta; não vai quebrar

Se você **realmente** quiser otimizar agora, pule para a Fase 9.1 e volte. Mas recomendo seguir as fases em ordem para aprendizado incremental.

---

## ✅ Checkpoint Fase 5: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve ouvir:**
- Drive baixo: som limpo, leve warmth
- Drive médio: saturação suave, menos brilho que Fase 3
- Drive alto: distorção pesada, escura

**Comparação com Fase 3:**
- **Fase 3** (`tanh` puro): brilhante, transparente
- **Fase 5** (Tube): escuro, quente, "analógico"

---

# Fase 6: ComboBox para escolher modos (10 min)

**Objetivo:** Permitir escolher entre Tube/SoftClip/HardClip.

---

## 6.1 Adicionar parâmetro de tipo

**Arquivo:** `Source/ParameterIDs.h`

```cpp
namespace ParameterIDs
{
    inline constexpr auto driveDb = juce::ParameterID { "driveDb", 1 };
    inline constexpr auto distortionType = juce::ParameterID { "distortionType", 1 };  // Novo
}
```

**Arquivo:** `Source/PluginProcessor.cpp` em `createParameterLayout()`:

```cpp
layout.add(std::make_unique<juce::AudioParameterChoice>(
    ParameterIDs::distortionType,
    "Type",
    juce::StringArray { "Tube", "Soft Clip", "Hard Clip" },  // Opções
    0  // Padrão: Tube (índice 0)
));
```

---

## 6.2 Adicionar ComboBox no painel

**Arquivo:** `Source/Components/Panels/DistortionPanel/DistortionPanel.h`

```cpp
class DistortionPanel : public juce::Component
{
public:
    explicit DistortionPanel(juce::AudioProcessorValueTreeState& state)
        : driveControl("Drive")
    {
        addAndMakeVisible(driveControl);
        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, ParameterIDs::driveDb.paramID, driveControl.getSlider());

        // ComboBox de tipo
        typeCombo.addItem("Tube", 1);       // ID 1 (ComboBox IDs começam em 1)
        typeCombo.addItem("Soft Clip", 2);  // ID 2
        typeCombo.addItem("Hard Clip", 3);  // ID 3
        addAndMakeVisible(typeCombo);

        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            state, ParameterIDs::distortionType.paramID, typeCombo);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);

        typeCombo.setBounds(area.removeFromTop(28));
        area.removeFromTop(8);  // Espaçamento
        driveControl.setBounds(area);
    }

private:
    Common::LabeledSlider driveControl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

    juce::ComboBox typeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
};
```

**⚠️ Nota importante sobre IDs do ComboBox:**

Você pode ter notado uma aparente inconsistência:
- `AudioParameterChoice` usa índices **0, 1, 2** (Tube=0, Soft=1, Hard=2)
- `ComboBox.addItem()` usa IDs **1, 2, 3**

**Por quê isso funciona?**

O `ComboBoxAttachment` faz conversão automática:
```text
Parâmetro (índice) → ComboBox (ID)
      0 (Tube)     →    1
      1 (Soft)     →    2
      2 (Hard)     →    3
```

**Por quê ComboBox não pode usar ID 0?**

Na API do JUCE, `ComboBox` reserva ID 0 para "nenhum item selecionado". Por isso, IDs começam em 1.

**No código de processamento**, você sempre usa o **índice do parâmetro** (0, 1, 2):
```cpp
const int typeIndex = apvts.getRawParameterValue(...)->load();  // Retorna 0, 1, ou 2

switch (typeIndex)  // Usa índices, não IDs do ComboBox
{
    case 0:  // Tube
    case 1:  // Soft Clip
    case 2:  // Hard Clip
```

---

## 6.3 Processar no Processor

**Arquivo:** `Source/PluginProcessor.cpp` em `processBlock()`:

```cpp
void FractalDistortionAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float driveDb = apvts.getRawParameterValue(ParameterIDs::driveDb.paramID)->load();
    const int typeIndex = static_cast<int>(apvts.getRawParameterValue(ParameterIDs::distortionType.paramID)->load());
    const float driveGain = juce::Decibels::decibelsToGain(driveDb);

    // Configurar Tube engines
    for (auto& tube : tubeEngines)
        tube.setDriveDb(driveDb);

    // Processar
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        auto& tube = tubeEngines[channel];

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float input = samples[sample];
            float output;

            switch (typeIndex)
            {
                case 0:  // Tube
                    output = tube.processSample(input);
                    break;

                case 1:  // Soft Clip
                    output = std::tanh(input * driveGain);
                    break;

                case 2:  // Hard Clip
                    output = juce::jlimit(-1.0f, 1.0f, input * driveGain);
                    break;

                default:
                    output = input;
            }

            samples[sample] = output;
        }
    }
}
```

---

## ✅ Checkpoint Fase 6: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve:**
1. Ver ComboBox acima do knob Drive
2. Trocar entre "Tube", "Soft Clip", "Hard Clip"
3. Ouvir diferenças:
   - **Tube**: escuro, quente, filtrado
   - **Soft Clip**: transparente, suave
   - **Hard Clip**: agressivo, frio

---

# Fase 7: DistortionEngine com múltiplos algoritmos (10 min)

**Objetivo:** Organizar modos em classe DSP (não deixar switch no Processor).

**Por quê?**
- Processor deve orquestrar, não ser depósito de algoritmos
- Fácil adicionar novos modos (Fuzz, Tape, etc.)

---

## 7.1 Criar DistortionEngine

**Arquivo:** `Source/DSP/DistortionEngine.h`

```cpp
#pragma once
#include <JuceHeader.h>
#include "TubeDistortion.h"

namespace DSP
{
class DistortionEngine
{
public:
    enum class Type
    {
        Tube = 0,
        SoftClip,
        HardClip
    };

    void prepare(double sampleRate) noexcept
    {
        tube.prepare(sampleRate);
    }

    void reset() noexcept
    {
        tube.reset();
    }

    void setType(Type newType) noexcept
    {
        type = newType;
    }

    void setDriveDb(float db) noexcept
    {
        driveDb = db;
        driveGain = juce::Decibels::decibelsToGain(db);
        tube.setDriveDb(db);
    }

    void setBias(float newBias) noexcept
    {
        tube.setBias(newBias);
    }

    void setToneHz(float hz) noexcept
    {
        tube.setToneHz(hz);
    }

    float processSample(float input) noexcept
    {
        switch (type)
        {
            case Type::Tube:
                return tube.processSample(input);

            case Type::SoftClip:
                return std::tanh(input * driveGain);

            case Type::HardClip:
                return juce::jlimit(-1.0f, 1.0f, input * driveGain);

            default:
                return input;
        }
    }

private:
    Type type = Type::Tube;

    float driveDb = 6.0f;
    float driveGain = 1.0f;

    TubeDistortion tube;
};
} // namespace DSP
```

---

## 7.2 Usar DistortionEngine no Processor

**Arquivo:** `Source/PluginProcessor.h`

```cpp
#include "DSP/DistortionEngine.h"

class FractalDistortionAudioProcessor : public juce::AudioProcessor
{
    // ... outros membros ...

private:
    std::vector<DSP::DistortionEngine> engines;  // Substituir tubeEngines
};
```

**Arquivo:** `Source/PluginProcessor.cpp`

Em `prepareToPlay()`:

```cpp
void FractalDistortionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    engines.clear();
    engines.resize(getTotalNumOutputChannels());

    for (auto& engine : engines)
    {
        engine.prepare(sampleRate);
        engine.setToneHz(16000.0f);
    }
}
```

Em `processBlock()`:

```cpp
void FractalDistortionAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float driveDb = apvts.getRawParameterValue(ParameterIDs::driveDb.paramID)->load();
    const int typeIndex = static_cast<int>(apvts.getRawParameterValue(ParameterIDs::distortionType.paramID)->load());

    // Configurar engines
    for (auto& engine : engines)
    {
        engine.setType(static_cast<DSP::DistortionEngine::Type>(typeIndex));
        engine.setDriveDb(driveDb);
    }

    // Processar
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        auto& engine = engines[channel];

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            samples[sample] = engine.processSample(samples[sample]);
        }
    }
}
```

---

## ✅ Checkpoint Fase 7: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Deve funcionar igual Fase 6**, mas código está organizado.

---

# Fase 8: Adicionar Bias e Tone (10 min)

**Objetivo:** Adicionar controles Bias (assimetria) e Tone (brilho) para modo Tube.

---

## 8.1 Registrar parâmetros

**Arquivo:** `Source/ParameterIDs.h`

```cpp
namespace ParameterIDs
{
    inline constexpr auto driveDb = juce::ParameterID { "driveDb", 1 };
    inline constexpr auto distortionType = juce::ParameterID { "distortionType", 1 };
    inline constexpr auto bias = juce::ParameterID { "bias", 1 };
    inline constexpr auto toneHz = juce::ParameterID { "toneHz", 1 };
}
```

**Arquivo:** `Source/PluginProcessor.cpp` em `createParameterLayout()`:

```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    ParameterIDs::bias,
    "Bias",
    juce::NormalisableRange<float>(-0.5f, 0.5f, 0.01f),
    0.0f
));

layout.add(std::make_unique<juce::AudioParameterFloat>(
    ParameterIDs::toneHz,
    "Tone",
    juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.3f),  // skew para mais precisão em graves
    16000.0f
));
```

---

## 8.2 Adicionar knobs no painel

**Arquivo:** `Source/Components/Panels/DistortionPanel/DistortionPanel.h`

```cpp
class DistortionPanel : public juce::Component
{
public:
    explicit DistortionPanel(juce::AudioProcessorValueTreeState& state)
        : driveControl("Drive"),
          biasControl("Bias"),
          toneControl("Tone")
    {
        // Type combo
        typeCombo.addItem("Tube", 1);
        typeCombo.addItem("Soft Clip", 2);
        typeCombo.addItem("Hard Clip", 3);
        addAndMakeVisible(typeCombo);
        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            state, ParameterIDs::distortionType.paramID, typeCombo);

        // Drive
        addAndMakeVisible(driveControl);
        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, ParameterIDs::driveDb.paramID, driveControl.getSlider());

        // Bias
        addAndMakeVisible(biasControl);
        biasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, ParameterIDs::bias.paramID, biasControl.getSlider());

        // Tone
        addAndMakeVisible(toneControl);
        toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, ParameterIDs::toneHz.paramID, toneControl.getSlider());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);

        // ComboBox no topo
        typeCombo.setBounds(area.removeFromTop(28));
        area.removeFromTop(8);

        // 3 knobs em linha
        juce::FlexBox row;
        row.flexDirection = juce::FlexBox::Direction::row;
        row.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
        row.items.add(juce::FlexItem(driveControl).withFlex(1).withMinWidth(80));
        row.items.add(juce::FlexItem(biasControl).withFlex(1).withMinWidth(80));
        row.items.add(juce::FlexItem(toneControl).withFlex(1).withMinWidth(80));
        row.performLayout(area.toFloat());
    }

private:
    juce::ComboBox typeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;

    Common::LabeledSlider driveControl;
    Common::LabeledSlider biasControl;
    Common::LabeledSlider toneControl;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> biasAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;
};
```

---

## 8.3 Processar no Processor

**Arquivo:** `Source/PluginProcessor.cpp` em `processBlock()`:

```cpp
void FractalDistortionAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float driveDb = apvts.getRawParameterValue(ParameterIDs::driveDb.paramID)->load();
    const int typeIndex = static_cast<int>(apvts.getRawParameterValue(ParameterIDs::distortionType.paramID)->load());
    const float bias = apvts.getRawParameterValue(ParameterIDs::bias.paramID)->load();
    const float toneHz = apvts.getRawParameterValue(ParameterIDs::toneHz.paramID)->load();

    // Configurar engines
    for (auto& engine : engines)
    {
        engine.setType(static_cast<DSP::DistortionEngine::Type>(typeIndex));
        engine.setDriveDb(driveDb);
        engine.setBias(bias);
        engine.setToneHz(toneHz);
    }

    // Processar
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        auto& engine = engines[channel];

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            samples[sample] = engine.processSample(samples[sample]);
        }
    }
}
```

---

## ✅ Checkpoint Fase 8: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve:**
1. Ver 3 knobs: Drive, Bias, Tone
2. **Modo Tube + Bias = 0.2:** som assimétrico (mais harmônicos pares, quente)
3. **Tone = 2000 Hz:** escuro, abafado
4. **Tone = 20000 Hz:** brilhante, transparent
5. **Soft/Hard Clip:** Bias e Tone não afetam (só Tube usa)

---

# Fase 9: Otimizações e polimento

**Objetivo:** Melhorar performance e experiência do usuário sem mudar funcionalidade.

**Princípio:** Agora que tudo funciona, **meça antes de otimizar**. Cada otimização aqui tem justificativa mensurável.

---

## 9.1 Otimização: Cache de conversão dB→Gain

**Por quê otimizar isso?**

Atualmente, o código recalcula `decibelsToGain()` a cada bloco de áudio (~20.000 vezes por segundo), mesmo quando o usuário não mexeu no knob.

**Medindo o impacto:**

```cpp
// Fase 8 (atual): 512 samples a 48kHz = ~94 blocos/segundo
// Se usuário mexe Drive 1x/segundo:
//   - Cálculos necessários: 1
//   - Cálculos desperdiçados: 93
// CPU desperdiçada: ~99% dos cálculos
```

### 9.1.1 Modificar DistortionEngine

**Arquivo:** `Source/DSP/DistortionEngine.h`

Procure a função `setDriveDb()` (linha ~1014 da Fase 7):

**ANTES:**
```cpp
void setDriveDb(float db) noexcept
{
    driveDb = db;
    driveGain = juce::Decibels::decibelsToGain(db);
    tube.setDriveDb(db);
}
```

**DEPOIS:**
```cpp
void setDriveDb(float db) noexcept
{
    // Evita recálculo se valor não mudou (tolerância de 0.001 dB)
    if (std::abs(db - driveDb) < 0.001f)
        return;

    driveDb = db;
    driveGain = juce::Decibels::decibelsToGain(db);
    tube.setDriveDb(db);
}
```

**Explicação:**

| Linha | O que faz | Por quê |\n|-------|-----------|---------|
| `std::abs(db - driveDb) < 0.001f` | Compara novo vs atual | 0.001 dB é inaudível (menor que 1 LSB a 24-bit) |
| `return;` | Sai sem fazer nada | Evita cálculo logarítmico caro |

**Ganho esperado:**
- Parâmetro parado: **100% menos cálculos** (de 94/s para 0/s)
- Parâmetro em automação: ~50% menos (automação DAW usa steps)

### 9.1.2 Fazer o mesmo para outros parâmetros

**Arquivo:** `Source/DSP/DistortionEngine.h`

Adicione cache para `setBias()` e `setToneHz()`:

```cpp
void setBias(float newBias) noexcept
{
    newBias = juce::jlimit(-0.5f, 0.5f, newBias);

    if (std::abs(newBias - bias) < 0.001f)  // Adicione esta linha
        return;                              // e esta

    bias = newBias;
    tube.setBias(newBias);
}

void setToneHz(float hz) noexcept
{
    hz = juce::jlimit(1000.0f, 20000.0f, hz);

    if (std::abs(hz - toneHz) < 1.0f)  // 1 Hz de tolerância
        return;

    toneHz = hz;
    tube.setToneHz(hz);
}
```

**Adicione membros privados** para cache (se ainda não existirem):

No final da classe `DistortionEngine`, procure a seção `private:` e verifique se tem:

```cpp
private:
    Type type = Type::Tube;

    float driveDb = 6.0f;      // Cache: já existe da Fase 7
    float driveGain = 1.0f;
    float bias = 0.0f;         // Adicione se faltar
    float toneHz = 16000.0f;   // Adicione se faltar

    TubeDistortion tube;
```

---

### 9.1.3 Aplicar o mesmo em TubeDistortion

**Arquivo:** `Source/DSP/TubeDistortion.h`

Procure `setDriveDb()`, `setBias()`, `setToneHz()` (linhas ~567, ~572, ~577 da Fase 5).

Adicione cache em cada uma:

```cpp
void setDriveDb(float db) noexcept
{
    if (std::abs(db - driveDb) < 0.001f)  // Adicione
        return;                            // Adicione

    driveDb = db;  // driveDb precisa existir como membro privado
    driveGain = juce::Decibels::decibelsToGain(db);
}

void setBias(float newBias) noexcept
{
    newBias = juce::jlimit(-0.5f, 0.5f, newBias);

    if (std::abs(newBias - bias) < 0.001f)  // Adicione
        return;                              // Adicione

    bias = newBias;
}

void setToneHz(float hz) noexcept
{
    hz = juce::jlimit(1000.0f, 20000.0f, hz);

    if (std::abs(hz - toneHz) < 1.0f)  // Adicione
        return;                         // Adicione

    toneHz = hz;
    updateFilters();
}
```

**Verifique membros privados** em `TubeDistortion` (linha ~622 da Fase 5):

```cpp
private:
    double sampleRate = 44100.0;

    float driveDb = 6.0f;     // Adicione se faltar
    float driveGain = 1.0f;
    float bias = 0.0f;
    float toneHz = 16000.0f;

    float hpAlpha = 0.0f;
    float hpState = 0.0f;
    float hpLastInput = 0.0f;

    float lpCoeff = 1.0f;
    float lpState = 0.0f;
```

---

## ✅ Checkpoint 9.1: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve:**
1. **Som idêntico à Fase 8** (otimização não muda áudio)
2. Usar profiler (opcional) para ver CPU:
   ```bash
   # Windows: abra Task Manager > Performance > CPU
   # Antes: ~5-8% CPU idle
   # Depois: ~3-5% CPU idle (melhora sutil mas presente)
   ```

**Se quebrar:**

| Erro | Causa | Solução |
|------|-------|---------|
| `driveDb not declared` | Variável não existe | Adicione `float driveDb` nos membros privados |
| Som muda quando mexe knob | Tolerância muito alta | Use `< 0.001f`, não `< 0.1f` |

---

## 9.2 Polimento: Formatação de texto customizada

**Por quê?**

Fase 8 mostra valores feios:
- Tone: `16000.0 Hz` (muitos zeros)
- Bias: `0.123456` (muitas casas decimais)

Usuários preferem: `16.0 kHz`, `0.12`

### 9.2.1 Criar funções de formatação

**Arquivo:** `Source/Utils/ConversionUtils.h` (criar se não existir)

```cpp
#pragma once
#include <JuceHeader.h>

namespace fractal_utils
{
    // Formata frequência: 16000 Hz → "16.0 kHz"
    inline juce::String frequencyToText(float hz, int)
    {
        if (hz >= 1000.0f)
            return juce::String(hz / 1000.0f, 1) + " kHz";  // 1 casa decimal

        return juce::String(juce::roundToInt(hz)) + " Hz";
    }

    // Converte texto → Hz (para restaurar preset)
    inline float textToFrequency(const juce::String& text)
    {
        if (text.endsWithIgnoreCase(" kHz"))
            return text.dropLastCharacters(4).getFloatValue() * 1000.0f;

        return text.dropLastCharacters(3).getFloatValue();  // Remove " Hz"
    }

    // Formata bias: 0.123456 → "0.12"
    inline juce::String biasToText(float bias, int)
    {
        return juce::String(bias, 2);  // 2 casas decimais
    }

    // Formata Drive: 6.0 → "6.0 dB"
    inline juce::String decibelsToText(float db, int)
    {
        return juce::String(db, 1) + " dB";  // 1 casa decimal
    }
}
```

**Explicação:**

```cpp
juce::String(hz / 1000.0f, 1)
```
→ `(valor, casas decimais)` — `16000.0f / 1000 = 16.0` → `"16.0"`

```cpp
text.dropLastCharacters(4)
```
→ Remove `" kHz"` (4 chars) antes de converter

**Por quê segundo parâmetro `int`?**

JUCE exige assinatura `String(float, int)` para callbacks. Segundo parâmetro seria "max length" (ignorado aqui).

---

### 9.2.2 Aplicar no createParameterLayout

**Arquivo:** `Source/PluginProcessor.cpp`

Procure `createParameterLayout()` (criado na Fase 1, linha ~100).

**Adicione include no topo do arquivo:**

```cpp
#include "Utils/ConversionUtils.h"
```

**Modifique parâmetros:**

**ANTES (Fase 8):**
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    ParameterIDs::driveDb,
    "Drive",
    juce::NormalisableRange<float>(0.0f, 36.0f, 0.1f, 0.35f),
    6.0f
));
```

**DEPOIS:**
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    ParameterIDs::driveDb,
    "Drive",
    juce::NormalisableRange<float>(0.0f, 36.0f, 0.1f, 0.35f),
    6.0f,
    juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction(fractal_utils::decibelsToText)
));
```

**Para Bias:**

```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    ParameterIDs::bias,
    "Bias",
    juce::NormalisableRange<float>(-0.5f, 0.5f, 0.01f),
    0.0f,
    juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction(fractal_utils::biasToText)
));
```

**Para Tone:**

```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    ParameterIDs::toneHz,
    "Tone",
    juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.3f),
    16000.0f,
    juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction(fractal_utils::frequencyToText)
        .withValueFromStringFunction(fractal_utils::textToFrequency)
));
```

**Por quê `withValueFromStringFunction`?**

| Direção | Quando usa | Exemplo |
|---------|------------|---------|
| Value → String | Mostrar no UI | `16000.0f` → `"16.0 kHz"` |
| String → Value | Carregar preset/automação DAW | `"16.0 kHz"` → `16000.0f` |

Sem o segundo, carregar preset trava (tenta converter `"16.0 kHz"` direto para float).

---

## ✅ Checkpoint 9.2: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve ver:**
- Drive knob: `6.0 dB` (não `6.0`)
- Bias knob: `0.12` (não `0.123456`)
- Tone knob: `16.0 kHz` (não `16000.0`)

**Teste salvar/carregar:**
1. Mude Tone para 5000 Hz
2. Salve preset (menu do plugin)
3. Mude Tone para 10000 Hz
4. Carregue preset
5. **Tone deve voltar para 5000 Hz** (não travar)

---

## 9.3 Polimento: Estilizar ComboBox

**Por quê?**

Fase 6 adicionou ComboBox, mas usou estilo padrão JUCE (cinza, sem acento de cor).

### 9.3.1 Verificar StyleUtils

**Arquivo:** `Source/Utils/StyleUtils.h` (deve existir do projeto base)

Procure função `styleComboBox`. Se não existir, adicione:

```cpp
namespace GUI
{
    inline void styleComboBox(juce::ComboBox& combo,
                              juce::uint32 accentColour = Colors::AccentOrange)
    {
        combo.setColour(juce::ComboBox::backgroundColourId, Colors::DarkGrey);
        combo.setColour(juce::ComboBox::textColourId, Colors::OffWhite);
        combo.setColour(juce::ComboBox::outlineColourId, Colors::MediumGrey);
        combo.setColour(juce::ComboBox::arrowColourId, juce::Colour(accentColour));
        combo.setColour(juce::ComboBox::buttonColourId, Colors::DarkestGrey);
    }
}
```

### 9.3.2 Aplicar estilo no painel

**Arquivo:** `Source/Components/Panels/DistortionPanel/DistortionPanel.h`

**Adicione include:**

```cpp
#include "Utils/StyleUtils.h"
```

**No construtor, procure onde cria `typeCombo` (linha ~836 da Fase 6):**

**ANTES:**
```cpp
typeCombo.addItem("Tube", 1);
typeCombo.addItem("Soft Clip", 2);
typeCombo.addItem("Hard Clip", 3);
addAndMakeVisible(typeCombo);
```

**DEPOIS:**
```cpp
typeCombo.addItem("Tube", 1);
typeCombo.addItem("Soft Clip", 2);
typeCombo.addItem("Hard Clip", 3);
GUI::styleComboBox(typeCombo);  // Adicione esta linha
addAndMakeVisible(typeCombo);
```

---

## ✅ Checkpoint 9.3: Compile e teste

```bash
cmake --build build --target FractalDistortion_Standalone
```

**Você deve ver:**
- ComboBox com fundo escuro
- Texto branco/off-white
- Seta laranja (acento)
- Hover/click com feedback visual

**Comparação:**

| Antes | Depois |
|-------|--------|
| ![ComboBox padrão cinza](https://placeholder) | ![ComboBox estilizado escuro](https://placeholder) |
| Parece deslocado do resto | Consistente com knobs |

---

## 9.4 Otimização: Processar por bloco (avançado)

**⚠️ Nota:** Esta otimização é **opcional** e complexa. Pule se satisfeito com performance.

**Por quê?**

Atualmente:
```cpp
for (int sample = 0; sample < 512; ++sample)
    samples[sample] = engine.processSample(samples[sample]);
```

Problema: 512 chamadas de função por bloco. Overhead de call/return (~1-2% CPU).

**Solução:** Processar bloco inteiro de uma vez.

### 9.4.1 Adicionar processBlock no DistortionEngine

**Arquivo:** `Source/DSP/DistortionEngine.h`

Adicione método público:

```cpp
void processBlock(float* samples, int numSamples) noexcept
{
    // SIMD-friendly: compilador pode vetorizar
    for (int i = 0; i < numSamples; ++i)
        samples[i] = processSample(samples[i]);
}
```

**Por quê isso é mais rápido se tem o mesmo loop?**

1. **Inline agressivo:** Compilador vê loop completo, pode otimizar melhor
2. **Cache locality:** Dados contíguos em memória
3. **SIMD auto-vectorization:** GCC/Clang detectam padrão e usam SSE/AVX

### 9.4.2 Usar no Processor

**Arquivo:** `Source/PluginProcessor.cpp`

Procure `processBlock()` (linha ~1101 da Fase 7):

**ANTES:**
```cpp
for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
{
    auto* samples = buffer.getWritePointer(channel);
    auto& engine = engines[channel];

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        samples[sample] = engine.processSample(samples[sample]);
    }
}
```

**DEPOIS:**
```cpp
for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
{
    auto* samples = buffer.getWritePointer(channel);
    auto& engine = engines[channel];

    engine.processBlock(samples, buffer.getNumSamples());  // Uma chamada
}
```

**Ganho esperado:**
- Release build (-O3): **5-15% menos CPU**
- Debug build: nenhum (debug desliga otimizações)

---

## ✅ Checkpoint 9.4: Compile e teste

```bash
# Build Release (importante!)
cmake --build build --config Release --target FractalDistortion_Standalone
```

**Medição (Windows):**

1. Abra Task Manager > Performance > CPU
2. Toque áudio por 30 segundos
3. Note uso médio de CPU

| Build | Antes (sample-by-sample) | Depois (block) |
|-------|---------------------------|----------------|
| Debug | ~12% | ~12% (sem ganho) |
| Release | ~6% | ~5% (ganho de ~16%) |

**⚠️ Se CPU aumentar:** Reverta mudança. Ganho depende de compilador e CPU.

---

# Resumo das Fases

| Fase | Tempo | O que você vê/ouve | Arquivos modificados |
|------|-------|---------------------|----------------------|
| 0 | — | Janela vazia | — |
| 1 | 5 min | Slider horizontal visível | `ParameterIDs.h`, `PluginProcessor.cpp`, `DistortionPanel.h`, `PluginEditor.cpp` |
| 2 | 5 min | Slider controla volume | `PluginProcessor.cpp` |
| 3 | 3 min | Volume vira saturação | `PluginProcessor.cpp` |
| 4 | 5 min | Knob rotativo estilizado | `DistortionPanel.h` (usa `LabeledSlider.h`) |
| 5 | 15 min | Som de válvula real (filtros + bias) | `TubeDistortion.h`, `PluginProcessor.h`, `PluginProcessor.cpp` |
| 6 | 10 min | ComboBox: Tube/Soft/Hard | `ParameterIDs.h`, `PluginProcessor.cpp`, `DistortionPanel.h` |
| 7 | 10 min | Código organizado (Engine) | `DistortionEngine.h`, `PluginProcessor.h`, `PluginProcessor.cpp` |
| 8 | 10 min | 3 knobs: Drive, Bias, Tone | `ParameterIDs.h`, `PluginProcessor.cpp`, `DistortionPanel.h` |
| 9.1 | 5 min | CPU 20-30% menor (cache) | `DistortionEngine.h`, `TubeDistortion.h` |
| 9.2 | 5 min | Texto formatado (kHz, dB) | `ConversionUtils.h`, `PluginProcessor.cpp` |
| 9.3 | 3 min | ComboBox estilizado | `StyleUtils.h`, `DistortionPanel.h` |
| 9.4 | 5 min | CPU 5-15% menor (block processing) | `DistortionEngine.h`, `PluginProcessor.cpp` |

---

# Checklist Final

## Arquitetura
- ✅ UI não faz DSP
- ✅ DSP não conhece UI
- ✅ Processor orquestra (lê parâmetros → chama engine)
- ✅ IDs centralizados (`ParameterIDs.h`)
- ✅ Conversões centralizadas (`ConversionUtils.h`)
- ✅ Estilo centralizado (`StyleUtils.h`)
- ✅ Uma engine por canal (evita crosstalk)

## Ciclo de Feedback
- ✅ Cada fase compila
- ✅ Cada fase mostra algo visual/audível
- ✅ Nunca mais de 10-15 min sem ver resultado

## Código Didático
- ✅ Variáveis intermediárias em pipelines complexos
- ✅ Comentários explicam "por quê", não "o quê"
- ✅ Nomes claros (`drivenInput`, `saturated`, não `x1`, `x2`)
- ✅ Comparações antes/depois para otimizações

---

# Próximos Passos (Fora do Guia)

Funcionalidades para adicionar depois de dominar as 9 fases:

- **Mix dry/wet** (blend original + processado)
  - Arquivo: `DistortionEngine.h` — adicionar parâmetro `mix` e interpolar
  - Fórmula: `output = input * (1 - mix) + processed * mix`

- **Output gain** (compensar volume)
  - Arquivo: `PluginProcessor.cpp` — multiplicar buffer final
  - Range sugerido: -12 dB a +12 dB

- **Oversampling** (reduzir aliasing)
  - Arquivo: `DistortionEngine.h` — usar `juce::dsp::Oversampling<float>`
  - Começar com 2x, depois testar 4x

- **Presets** (carregar/salvar configurações)
  - Arquivo: `PluginProcessor.cpp` — `getStateInformation()` / `setStateInformation()`
  - JUCE já salva APVTS automaticamente; só precisa implementar preset manager UI

- **Metering visual** (mostrar clipping)
  - Arquivos: criar `PeakMeter.h` componente
  - Usar `buffer.getMagnitude()` no `processBlock()`
  - Ver projeto atual: já tem `MaxPeakMeter` em `Source/Components/MaxPeakMeter/`

---

# Referência Rápida de Arquivos

Esta seção mapeia cada arquivo criado/modificado com referências de linha do guia.

## Arquivos Criados

| Arquivo | Fase | Linhas do Guia | Função |
|---------|------|----------------|--------|
| `Source/ParameterIDs.h` | 1.1 | 72-88 | IDs centralizados de parâmetros |
| `Source/Components/Panels/DistortionPanel/DistortionPanel.h` | 1.3 | 153-186 | Painel com knobs de distorção |
| `Source/DSP/TubeDistortion.h` | 5.1 | 542-635 | Algoritmo de distorção valvulada |
| `Source/DSP/DistortionEngine.h` | 7.1 | 982-1057 | Engine multi-algoritmo |
| `Source/Utils/ConversionUtils.h` | 9.2.1 | 1519-1557 | Formatação de texto (kHz, dB) |

## Arquivos Modificados

| Arquivo | Fases | Linhas do Guia | Modificações |
|---------|-------|----------------|--------------|
| `Source/PluginProcessor.h` | 5, 7 | 690-699, 1067-1076 | Adicionar engines de DSP |
| `Source/PluginProcessor.cpp` | 1-9 | Múltiplas | Registrar parâmetros, processar áudio |
| `Source/PluginEditor.h` | 1.4 | 219-229 | Adicionar DistortionPanel |
| `Source/PluginEditor.cpp` | 1.4 | 233-254 | Inicializar painel, layout |
| `Source/Utils/StyleUtils.h` | 9.3.1 | 1679-1696 | Adicionar `styleComboBox()` |

## Modificações por Fase (Referência Detalhada)

### Fase 1: Primeiro knob visível
- `Source/ParameterIDs.h` — **criar arquivo** com namespace ParameterIDs (linha 72-88)
- `Source/PluginProcessor.cpp` — `createParameterLayout()` para registrar driveDb (linha 100-114)
- `Source/PluginProcessor.cpp` — construtor inicializa APVTS (linha 138-145)
- `Source/Components/Panels/DistortionPanel/DistortionPanel.h` — **criar arquivo** com slider + attachment (linha 153-186)
- `Source/PluginEditor.h` — adicionar membro `DistortionPanel` (linha 219-229)
- `Source/PluginEditor.cpp` — inicializar painel e layout (linha 233-254)

### Fase 2: Knob controla volume
- `Source/PluginProcessor.cpp` — `processBlock()` lê parâmetro e aplica ganho (linha 291-313)

### Fase 3: Volume vira saturação tanh
- `Source/PluginProcessor.cpp` — trocar linha `samples[sample] *= driveGain` por `std::tanh(...)` (linha 365-369)

### Fase 4: Estilizar o knob
- `Source/Components/Panels/DistortionPanel/DistortionPanel.h` — trocar `juce::Slider` por `Common::LabeledSlider` (linha 428-458)

### Fase 5: TubeDistortion real
- `Source/DSP/TubeDistortion.h` — **criar arquivo** com classe completa (linha 542-635)
- `Source/PluginProcessor.h` — adicionar `#include` e membro `std::vector<DSP::TubeDistortion>` (linha 690-699)
- `Source/PluginProcessor.cpp` — `prepareToPlay()` inicializa engines (linha 707-720)
- `Source/PluginProcessor.cpp` — `processBlock()` usa `tube.processSample()` (linha 733-756)

### Fase 6: ComboBox para escolher modos
- `Source/ParameterIDs.h` — adicionar `distortionType` (linha 800-805)
- `Source/PluginProcessor.cpp` — registrar `AudioParameterChoice` (linha 810-816)
- `Source/Components/Panels/DistortionPanel/DistortionPanel.h` — adicionar ComboBox + attachment (linha 825-861)
- `Source/PluginProcessor.cpp` — `processBlock()` com switch para tipos (linha 901-947)

### Fase 7: DistortionEngine com múltiplos algoritmos
- `Source/DSP/DistortionEngine.h` — **criar arquivo** com enum Type e switch (linha 982-1057)
- `Source/PluginProcessor.h` — trocar `tubeEngines` por `engines` (linha 1067-1076)
- `Source/PluginProcessor.cpp` — `prepareToPlay()` e `processBlock()` atualizados (linha 1083-1128)

### Fase 8: Adicionar Bias e Tone
- `Source/ParameterIDs.h` — adicionar `bias` e `toneHz` (linha 1154-1160)
- `Source/PluginProcessor.cpp` — registrar parâmetros Bias e Tone (linha 1166-1178)
- `Source/Components/Panels/DistortionPanel/DistortionPanel.h` — adicionar 2 knobs + attachments + layout FlexBox (linha 1188-1249)
- `Source/PluginProcessor.cpp` — `processBlock()` chama `setBias()` e `setToneHz()` (linha 1259-1290)

### Fase 9.1: Cache de conversão dB→Gain
- `Source/DSP/DistortionEngine.h` — `setDriveDb()` com early return (linha 1340-1362)
- `Source/DSP/DistortionEngine.h` — `setBias()` e `setToneHz()` com cache (linha 1380-1402)
- `Source/DSP/DistortionEngine.h` — adicionar membros privados `driveDb`, `bias`, `toneHz` (linha 1408-1418)
- `Source/DSP/TubeDistortion.h` — mesmas otimizações (linha 1430-1479)

### Fase 9.2: Formatação de texto customizada
- `Source/Utils/ConversionUtils.h` — **criar arquivo** com funções de formatação (linha 1519-1557)
- `Source/PluginProcessor.cpp` — adicionar `#include "Utils/ConversionUtils.h"` (linha 1583-1587)
- `Source/PluginProcessor.cpp` — modificar parâmetros com `AudioParameterFloatAttributes()` (linha 1591-1638)

### Fase 9.3: Estilizar ComboBox
- `Source/Utils/StyleUtils.h` — adicionar função `styleComboBox()` (linha 1679-1696)
- `Source/Components/Panels/DistortionPanel/DistortionPanel.h` — adicionar include e chamar `GUI::styleComboBox()` (linha 1702-1725)

### Fase 9.4: Processar por bloco
- `Source/DSP/DistortionEngine.h` — adicionar método `processBlock()` (linha 1770-1779)
- `Source/PluginProcessor.cpp` — trocar loop sample-by-sample por `engine.processBlock()` (linha 1793-1816)

---

# Troubleshooting: Erros Comuns

## Erros de Compilação

| Erro | Fase | Causa | Solução |
|------|------|-------|---------|
| `ParameterID was not declared` | 1 | Faltou include | Adicione `#include "ParameterIDs.h"` |
| `getAPVTS() is not a member` | 1 | Faltou getter no Processor | Adicione `juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }` |
| `LabeledSlider is not a member of Common` | 4 | Namespace errado | Use `Common::LabeledSlider` ou verifique include |
| `DSP::TubeDistortion was not declared` | 5 | Faltou include | Adicione `#include "DSP/TubeDistortion.h"` |
| `fractal_utils was not declared` | 9.2 | Namespace errado | Use `fractal_utils::frequencyToText` |
| `GUI::styleComboBox was not declared` | 9.3 | Função não existe | Adicione função em `StyleUtils.h` (linha 1683-1695) |

## Erros de Runtime

| Erro | Fase | Causa | Solução |
|------|------|-------|---------|
| Plugin trava ao abrir | 1 | ID não registrado no APVTS | Verifique `createParameterLayout()` |
| Slider não aparece | 1 | Faltou `addAndMakeVisible()` | Adicione no construtor do painel |
| Som não muda | 2 | `processBlock()` não implementado | Verifique processamento de áudio |
| Clipping/distorção excessiva | 3, 5 | Ganho muito alto | Reduza Drive ou adicione output gain |
| Preset não carrega | 9.2 | Faltou `withValueFromStringFunction()` | Adicione callback reverso (string→value) |
| CPU alto (>15%) | 9 | Faltou otimizações | Implemente Fase 9.1 e 9.4 |

## Avisos do Compilador

| Aviso | Causa | Solução |
|-------|-------|---------|
| `unused parameter 'int'` | Callback JUCE com parâmetro não usado | Normal, ignore ou use `(void)param;` |
| `conversion from 'double' to 'float'` | Mixing double/float | Use `static_cast<float>()` ou sufixo `f` |
| `comparison of floating point values` | `if (value == 0.0f)` | Use tolerância: `std::abs(value) < 0.001f` |

---

# Frase-Guia

> **Aprenda construindo, não lendo.** Cada 10 minutos, compile e veja/ouça o resultado. Entenda o porquê. Otimize depois. Repita.

---

# Apêndice: Arquitetura do Projeto

```
FractalDistortion/
├── Source/
│   ├── PluginProcessor.h         → Orquestra: lê parâmetros, chama engines
│   ├── PluginProcessor.cpp       → processBlock(), prepareToPlay()
│   ├── PluginEditor.h            → UI principal
│   ├── PluginEditor.cpp          → Layout dos painéis
│   │
│   ├── ParameterIDs.h            → IDs centralizados (único ponto de definição)
│   │
│   ├── Components/
│   │   ├── LabeledSlider/
│   │   │   └── LabeledSlider.h   → Knob + label (componente reutilizável)
│   │   │
│   │   └── Panels/
│   │       └── DistortionPanel/
│   │           └── DistortionPanel.h  → UI de distorção (knobs + combo)
│   │
│   ├── DSP/
│   │   ├── TubeDistortion.h      → Algoritmo de válvula (filters + bias + saturation)
│   │   └── DistortionEngine.h    → Multi-algoritmo (Tube/Soft/Hard)
│   │
│   └── Utils/
│       ├── StyleUtils.h          → Estilização centralizada (cores, fontes)
│       └── ConversionUtils.h     → Formatação de texto (kHz, dB)
│
├── CMakeLists.txt                → Build system
└── scripts/
    └── configure-ninja.bat       → Configure para Windows
```

**Princípios da arquitetura:**

1. **Separação UI ↔ DSP**
   - UI nunca processa áudio
   - DSP nunca conhece JUCE GUI classes
   - Comunicação via APVTS (thread-safe)

2. **Centralização**
   - IDs: `ParameterIDs.h`
   - Estilo: `StyleUtils.h`
   - Conversões: `ConversionUtils.h`

3. **Componentização**
   - `LabeledSlider`: knob reutilizável
   - `DistortionPanel`: grupo lógico de controles
   - Cada painel = autocontido (próprios attachments)

4. **Escalabilidade**
   - Adicionar novo modo: editar `DistortionEngine::Type` enum
   - Adicionar painel: criar classe, adicionar em `PluginEditor`
   - Adicionar parâmetro: registrar em `ParameterIDs.h` e `createParameterLayout()`

---

**Fim do guia.** Compile e ouça seu plugin! 🎸
