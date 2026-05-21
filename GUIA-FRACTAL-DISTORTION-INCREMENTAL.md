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

Agora que tudo funciona, **questione cada parte**:

## 9.1 Otimização: Cache de conversão dB→Gain

**Problema:**
```cpp
// A cada bloco, recalcula mesmo se Drive não mudou
engine.setDriveDb(driveDb);  // Dentro: decibelsToGain()
```

**Solução:**
```cpp
// No DistortionEngine, só recalcula se valor mudou
void setDriveDb(float db) noexcept
{
    if (std::abs(db - driveDb) < 0.001f) return;  // Evita recálculo

    driveDb = db;
    driveGain = juce::Decibels::decibelsToGain(db);
    tube.setDriveDb(db);
}
```

---

## 9.2 Otimização: Processar por bloco (não sample-by-sample)

**Problema:**
```cpp
for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
{
    samples[sample] = engine.processSample(samples[sample]);  // Muitas chamadas de função
}
```

**Solução:** Adicionar método `processBlock` no DSP:

```cpp
void processBlock(float* samples, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
        samples[i] = processSample(samples[i]);
}
```

Ganho: ~10-20% menos overhead de chamadas de função.

---

## 9.3 Polimento: Formatação de texto customizada

**Problema:** Tone mostra "16000.0 Hz" → feio.

**Solução:** Criar `frequencyToText` em `ConversionUtils.h`:

```cpp
namespace fractal_utils
{
    inline juce::String frequencyToText(float hz, int)
    {
        if (hz >= 1000.0f)
            return juce::String(hz / 1000.0f, 1) + " kHz";
        return juce::String(juce::roundToInt(hz)) + " Hz";
    }
}
```

Usar no parâmetro:

```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    ParameterIDs::toneHz,
    "Tone",
    juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.3f),
    16000.0f,
    juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction(fractal_utils::frequencyToText)
));
```

---

## 9.4 Polimento: Estilizar ComboBox

**Arquivo:** `Source/Components/Panels/DistortionPanel/DistortionPanel.h`

No construtor, após criar `typeCombo`:

```cpp
GUI::styleComboBox(typeCombo);  // Usa StyleUtils
```

---

# Resumo das Fases

| Fase | Tempo | O que você vê/ouve | Arquivo chave |
|------|-------|---------------------|---------------|
| 0 | — | Janela vazia | — |
| 1 | 5 min | Slider horizontal visível | `DistortionPanel.h` |
| 2 | 5 min | Slider controla volume | `PluginProcessor.cpp` |
| 3 | 3 min | Volume vira saturação | `PluginProcessor.cpp` |
| 4 | 5 min | Knob rotativo estilizado | `LabeledSlider.h` |
| 5 | 15 min | Som de válvula real (filtros + bias) | `TubeDistortion.h` |
| 6 | 10 min | ComboBox: Tube/Soft/Hard | `DistortionPanel.h` |
| 7 | 10 min | Código organizado (Engine) | `DistortionEngine.h` |
| 8 | 10 min | 3 knobs: Drive, Bias, Tone | `DistortionPanel.h` |
| 9 | — | Otimizações + polish | Vários |

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

- **Mix dry/wet** (blend original + processado)
- **Output gain** (compensar volume)
- **Oversampling** (reduzir aliasing)
- **Presets** (carregar/salvar configurações)
- **Metering visual** (mostrar clipping)

---

# Frase-Guia

> **Aprenda construindo, não lendo.** Cada 10 minutos, compile e veja/ouça o resultado. Entenda o porquê. Otimize depois. Repita.
