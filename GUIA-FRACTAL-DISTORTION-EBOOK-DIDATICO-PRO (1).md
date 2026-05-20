# Fractal Distortion — Guia Didático Profissional em JUCE/C++

## Como usar este guia

Este guia deve funcionar como material de curso: cada etapa explica **onde criar/editar arquivos**, **o que cada linha importante faz**, **como testar na UI/áudio**, **onde pode quebrar**, **quando quebrar código em funções menores** e **o que deve virar utilitário reutilizável**.

A regra de qualidade é simples:

> Se uma etapa não ensina o porquê do código, ela ainda não está pronta.

---

## Índice

- [1. Mapa mental do projeto](#1-mapa-mental-do-projeto)
- [2. Organização de arquivos](#2-organização-de-arquivos)
- [3. Utilitários: o que não deve ficar espalhado](#3-utilitários-o-que-não-deve-ficar-espalhado)
- [4. `ParameterIDs.h`: IDs centralizados](#4-parameteridsh-ids-centralizados)
- [5. Criando parâmetros no APVTS](#5-criando-parâmetros-no-apvts)
- [6. UI em JUCE comparada com HTML/CSS/JS](#6-ui-em-juce-comparada-com-htmlcssjs)
- [7. `DistortionPanel`: primeiro painel testável](#7-distortionpanel-primeiro-painel-testável)
- [8. `LabeledSlider`: quando virar componente reutilizável](#8-labeledslider-quando-virar-componente-reutilizável)
- [9. Drive primeiro como volume, depois como saturação](#9-drive-primeiro-como-volume-depois-como-saturação)
- [10. Tube Distortion explicada de verdade](#10-tube-distortion-explicada-de-verdade)
- [11. `DistortionEngine`: organizando modos de distorção](#11-distortionengine-organizando-modos-de-distorção)
- [12. Onde o código pode quebrar](#12-onde-o-código-pode-quebrar)
- [13. O que vira utilitário, helper ou classe](#13-o-que-vira-utilitário-helper-ou-classe)
- [14. CMake](#14-cmake)
- [15. Checklist profissional](#15-checklist-profissional)

---

# 1. Mapa mental do projeto

Um plugin de distorção é um fluxo de sinal:

```text
Áudio da DAW
    ↓
Input Gain
    ↓
Drive
    ↓
Tipo de distorção
    ↓
Tone / Warmth
    ↓
Mix dry/wet
    ↓
Output Gain
    ↓
Áudio de volta para a DAW
```

O projeto deve refletir isso:

```text
UI → APVTS → Processor → DSP → Buffer de áudio
```

Cada camada tem uma função:

| Camada | Responsabilidade |
|---|---|
| UI | Mostrar sliders, knobs, botões e medidores |
| APVTS | Guardar parâmetros e sincronizar UI/host/áudio |
| Processor | Ler parâmetros e chamar o DSP |
| DSP | Processar áudio |
| Utils | Conversões, formatações e funções puras reutilizáveis |

Regra de ouro:

```text
UI não faz DSP.
DSP não conhece UI.
Processor orquestra.
Utils não dependem do estado do plugin.
```

---

# 2. Organização de arquivos

Estrutura recomendada:

```text
FractalDistortion/
├── CMakeLists.txt
└── Source/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    ├── PluginEditor.cpp
    ├── ParameterIDs.h
    ├── Utils/
    │   └── ConversionUtils.h
    ├── DSP/
    │   ├── TubeDistortion.h
    │   ├── TubeDistortion.cpp
    │   ├── DistortionEngine.h
    │   └── DistortionEngine.cpp
    └── Components/
        ├── Common/
        │   ├── LabeledSlider.h
        │   └── FractalLookAndFeel.h
        └── Panels/
            └── DistortionPanel/
                ├── DistortionPanel.h
                └── DistortionPanel.cpp
```

Tabela prática:

| Coisa | Onde fica |
|---|---|
| IDs de parâmetros | `Source/ParameterIDs.h` |
| Conversões dB/linear/texto | `Source/Utils/ConversionUtils.h` |
| Algoritmo Tube | `Source/DSP/TubeDistortion.h/.cpp` |
| Escolha entre Tube/SoftClip/HardClip | `Source/DSP/DistortionEngine.h/.cpp` |
| Painel visual da distorção | `Source/Components/Panels/DistortionPanel/` |
| Slider com label reutilizável | `Source/Components/Common/LabeledSlider.h` |
| Tema visual global | `Source/Components/Common/FractalLookAndFeel.h` |

---

# 3. Utilitários: o que não deve ficar espalhado

Você já tem um arquivo muito bom:

```text
Source/Utils/ConversionUtils.h
```

Ele centraliza lógica como:

- converter ganho linear para dB;
- formatar pico para medidor;
- converter percentual para texto;
- converter texto do usuário para valor de parâmetro.

Isso é exatamente o tipo de coisa que **não deve ficar espalhada** entre `PluginEditor`, `OutputPanel`, `DistortionPanel` e `PluginProcessor`.

## Exemplo seu, explicado

```cpp
inline float linearToMeterDb(float linearGain) noexcept
{
    if (linearGain < 1e-8f)
        return -60.f;

    return juce::jlimit(-60.f, 6.f, juce::Decibels::gainToDecibels(linearGain));
}
```

Linha por linha:

```cpp
inline float linearToMeterDb(float linearGain) noexcept
```

- `inline`: permite definir a função no header sem erro de múltipla definição.
- `float`: retorna número decimal simples.
- `linearToMeterDb`: nome claro; entrada linear, Output em dB para medidor.
- `float linearGain`: amplitude linear, por exemplo `0.5`, `1.0`, `1.2`.
- `noexcept`: garante que a função não lança exceção.

```cpp
if (linearGain < 1e-8f)
    return -60.f;
```

Essa proteção evita `-inf` ou valores absurdos no medidor.

Em áudio, silêncio digital pode virar menos infinito em dB. Para UI, isso geralmente é ruim. Então você escolhe um piso visual: `-60 dB`.

```cpp
return juce::jlimit(-60.f, 6.f, juce::Decibels::gainToDecibels(linearGain));
```

Essa linha faz duas coisas:

1. converte ganho linear para dB;
2. limita o resultado entre `-60` e `+6`.

Para aprendizado, eu quebraria assim:

```cpp
const float dbValue = juce::Decibels::gainToDecibels(linearGain);
const float clampedDb = juce::jlimit(-60.f, 6.f, dbValue);
return clampedDb;
```

Quando quebrar uma linha?

- quando ela faz mais de uma coisa;
- quando você quer debugar;
- quando o guia precisa ensinar;
- quando o nome da variável intermediária ajuda a contar a história.

Quando manter em uma linha?

- quando a função já está madura;
- quando a intenção está óbvia;
- quando não prejudica a leitura.

## O que mais pode entrar em `ConversionUtils.h`

```cpp
inline juce::String frequencyToText(float hz, int)
{
    if (hz >= 1000.0f)
        return juce::String(hz / 1000.0f, 1) + " kHz";

    return juce::String(juce::roundToInt(hz)) + " Hz";
}
```

Versão didática:

```cpp
inline juce::String frequencyToText(float hz, int)
{
    const bool shouldDisplayAsKhz = hz >= 1000.0f;

    if (shouldDisplayAsKhz)
    {
        const float khz = hz / 1000.0f;
        return juce::String(khz, 1) + " kHz";
    }

    const int roundedHz = juce::roundToInt(hz);
    return juce::String(roundedHz) + " Hz";
}
```

Isso pode ser usado em:

- `Tone`;
- `Warmth`;
- filtros;
- parâmetros exibidos no host.

---

# 4. `ParameterIDs.h`: IDs centralizados

Arquivo: `Source/ParameterIDs.h`  
Ação: criar.

```cpp
#pragma once

#include <JuceHeader.h>

namespace ParameterIDs
{
    inline constexpr auto driveDb = juce::ParameterID { "driveDb", 1 };
    inline constexpr auto outputGainDb = juce::ParameterID { "outputGainDb", 1 };
    inline constexpr auto mixPercent = juce::ParameterID { "mixPercent", 1 };
    inline constexpr auto bias = juce::ParameterID { "bias", 1 };
    inline constexpr auto toneHz = juce::ParameterID { "toneHz", 1 };
    inline constexpr auto distortionType = juce::ParameterID { "distortionType", 1 };
}
```

## Explicação

```cpp
#pragma once
```

Impede múltiplas inclusões do mesmo arquivo.

```cpp
#include <JuceHeader.h>
```

Necessário para `juce::ParameterID`.

```cpp
namespace ParameterIDs
```

Agrupa os IDs para evitar strings soltas.

```cpp
inline constexpr auto driveDb = juce::ParameterID { "driveDb", 1 };
```

Partes:

- `inline`: seguro em header.
- `constexpr`: constante de compilação.
- `auto`: compilador deduz `juce::ParameterID`.
- `driveDb`: nome usado no C++.
- `"driveDb"`: ID interno salvo em presets/sessões.
- `1`: versão do parâmetro.

Evite mudar `"driveDb"` depois que o plugin começar a ser usado em projetos reais. Isso pode quebrar presets e sessões antigas.

---

# 5. Criando parâmetros no APVTS

Arquivo: `Source/PluginProcessor.cpp`  
Ação: editar.  
Onde: dentro de `createParameterLayout()`.

Versão direta:

```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    ParameterIDs::driveDb,
    "Drive",
    juce::NormalisableRange<float>(0.0f, 36.0f, 0.1f, 0.35f),
    6.0f,
    juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction(fractal_utils::gainDbToText)
        .withValueFromStringFunction(fractal_utils::textToGainDb)
));
```

## Explicação linha por linha

```cpp
layout.add(...)
```

Adiciona um parâmetro ao layout do APVTS.

```cpp
std::make_unique<juce::AudioParameterFloat>
```

Cria um parâmetro float com ownership seguro.

```cpp
ParameterIDs::driveDb
```

Usa ID centralizado.

```cpp
"Drive"
```

Nome visível para usuário e DAW.

```cpp
juce::NormalisableRange<float>(0.0f, 36.0f, 0.1f, 0.35f)
```

Define:

- mínimo: `0 dB`;
- máximo: `36 dB`;
- passo: `0.1 dB`;
- skew: `0.35`.

O skew muda a sensação do controle. Para Drive, é bom ter mais precisão em valores baixos/médios.

```cpp
6.0f
```

Valor padrão.

```cpp
.withStringFromValueFunction(fractal_utils::gainDbToText)
```

Usa sua função utilitária para transformar valor em texto.

```cpp
.withValueFromStringFunction(fractal_utils::textToGainDb)
```

Permite o usuário digitar valor no host/UI e converter de volta.

## Quando quebrar em helper

Se vários parâmetros usam a mesma lógica, crie uma função local:

```cpp
namespace
{
    std::unique_ptr<juce::AudioParameterFloat> makeGainParameter(
        juce::ParameterID id,
        const juce::String& name,
        float minDb,
        float maxDb,
        float defaultDb)
    {
        const auto range = juce::NormalisableRange<float>(
            minDb,
            maxDb,
            0.1f,
            0.35f
        );

        const auto attributes = juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(fractal_utils::gainDbToText)
            .withValueFromStringFunction(fractal_utils::textToGainDb);

        return std::make_unique<juce::AudioParameterFloat>(
            id,
            name,
            range,
            defaultDb,
            attributes
        );
    }
}
```

Agora:

```cpp
layout.add(makeGainParameter(ParameterIDs::driveDb, "Drive", 0.0f, 36.0f, 6.0f));
layout.add(makeGainParameter(ParameterIDs::outputGainDb, "Output", -24.0f, 24.0f, 0.0f));
```

Isso melhora legibilidade e reduz duplicação.

Não colocaria isso em `ConversionUtils.h`, porque não é conversão. É fábrica de parâmetro. Se crescer muito, pode virar:

```text
Source/Parameters/ParameterFactory.h
```

---

# 6. UI em JUCE comparada com HTML/CSS/JS

Na web:

```text
HTML = estrutura
CSS  = visual
JS   = comportamento
```

No JUCE:

```text
Componentes = estrutura
paint()     = visual
resized()   = layout
Listeners   = comportamento
APVTS       = estado
Attachments = ligação entre UI e parâmetro
```

Exemplo web:

```html
<div class="control-row">
  <label>Drive</label>
  <input type="range" />
</div>
```

Equivalente JUCE:

```cpp
juce::Label driveLabel;
juce::Slider driveSlider;
```

CSS:

```css
.control-row {
  display: flex;
  gap: 8px;
  padding: 12px;
}
```

JUCE:

```cpp
auto area = getLocalBounds().reduced(12);
auto row = area.removeFromTop(28);

driveLabel.setBounds(row.removeFromLeft(70));
driveSlider.setBounds(row);
```

Diferença principal:

> No CSS o navegador calcula layout. No JUCE você geralmente calcula retângulos manualmente.

---

# 7. `DistortionPanel`: primeiro painel testável

Arquivo: `Source/Components/Panels/DistortionPanel/DistortionPanel.h`  
Ação: criar versão inicial.

```cpp
#pragma once

#include <JuceHeader.h>
#include "../../../ParameterIDs.h"

class DistortionPanel : public juce::Component
{
public:
    explicit DistortionPanel(juce::AudioProcessorValueTreeState& state)
    {
        driveLabel.setText("Drive", juce::dontSendNotification);
        driveLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(driveLabel);

        driveSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        driveSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
        addAndMakeVisible(driveSlider);

        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state,
            ParameterIDs::driveDb.getParamID(),
            driveSlider
        );
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);
        auto row = area.removeFromTop(28);

        driveLabel.setBounds(row.removeFromLeft(70));
        driveSlider.setBounds(row);
    }

private:
    juce::Label driveLabel;
    juce::Slider driveSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
};
```

## Explicação essencial

```cpp
class DistortionPanel : public juce::Component
```

Cria um componente visual.

```cpp
explicit DistortionPanel(...)
```

Construtor recebe o APVTS, porque precisa conectar slider ao parâmetro.

```cpp
addAndMakeVisible(driveLabel);
```

Sem isso, o label existe, mas não aparece.

```cpp
driveAttachment = std::make_unique<...>(...);
```

Conecta slider ao parâmetro.

Erro clássico:

```cpp
auto driveAttachment = std::make_unique<...>();
```

Isso cria uma variável local que morre no fim do construtor. O attachment precisa ser membro da classe.

```cpp
void resized() override
```

Lugar correto para layout.

```cpp
getLocalBounds().reduced(12)
```

Equivalente mental a `padding: 12px`.

```cpp
row.removeFromLeft(70)
```

Separa 70 px para o label.

---

# 8. `LabeledSlider`: quando virar componente reutilizável

Quando você repetir este padrão:

```text
Label + Slider
Label + Slider
Label + Slider
```

crie um componente.

Arquivo: `Source/Components/Common/LabeledSlider.h`  
Ação: criar.

```cpp
#pragma once

#include <JuceHeader.h>

class LabeledSlider : public juce::Component
{
public:
    explicit LabeledSlider(const juce::String& labelText)
    {
        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);

        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
        addAndMakeVisible(slider);
    }

    juce::Slider& getSlider() noexcept
    {
        return slider;
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromLeft(72));
        slider.setBounds(area);
    }

private:
    juce::Label label;
    juce::Slider slider;
};
```

O painel passa a ficar assim:

```cpp
LabeledSlider driveControl { "Drive" };
LabeledSlider biasControl  { "Bias" };
LabeledSlider toneControl  { "Tone" };
```

Isso melhora:

- clareza;
- manutenção;
- consistência visual;
- velocidade de desenvolvimento.

---

# 9. Drive primeiro como volume, depois como saturação

Arquivo: `Source/PluginProcessor.cpp`  
Ação: editar `processBlock()`.

## 9.1 Drive como volume

```cpp
const float driveDb = apvts
    .getRawParameterValue(ParameterIDs::driveDb.getParamID())
    ->load();

const float driveGain = juce::Decibels::decibelsToGain(driveDb);

for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
{
    auto* samples = buffer.getWritePointer(channel);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        samples[sample] *= driveGain;
    }
}
```

Explicação:

```cpp
getRawParameterValue(...)->load()
```

Lê o valor atual do parâmetro.

```cpp
Decibels::decibelsToGain(driveDb)
```

Converte dB para multiplicador linear.

```cpp
getWritePointer(channel)
```

Pega acesso editável às amostras daquele canal.

```cpp
samples[sample] *= driveGain;
```

Multiplica cada amostra.

Teste:

- o slider Drive deve aumentar volume;
- ainda não precisa soar bonito;
- objetivo é validar UI → APVTS → áudio.

## 9.2 Drive como saturação

Versão compacta:

```cpp
samples[sample] = std::tanh(samples[sample] * driveGain);
```

Versão didática melhor:

```cpp
const float inputSample = samples[sample];
const float drivenSample = inputSample * driveGain;
const float saturatedSample = std::tanh(drivenSample);

samples[sample] = saturatedSample;
```

Essa versão mostra o processo:

```text
amostra original → amostra amplificada → amostra saturada
```

Depois, quando estiver maduro, dá para compactar.

---

# 10. Tube Distortion explicada de verdade

Tube simples:

```text
Input
  ↓
High-pass leve
  ↓
Bias
  ↓
Drive
  ↓
tanh
  ↓
Correção de DC
  ↓
Low-pass / Tone
  ↓
Output
```

Arquivo: `Source/DSP/TubeDistortion.h`  
Ação: criar.

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
        const float filteredInput = highPass(input);
        const float biasedInput = filteredInput + bias;
        const float drivenInput = biasedInput * driveGain;
        const float saturated = std::tanh(drivenInput);
        const float dcOffset = std::tanh(bias * driveGain);
        const float corrected = saturated - dcOffset;
        const float toned = lowPass(corrected);

        return toned;
    }

private:
    void updateFilters() noexcept
    {
        const float hpHz = 25.0f;

        hpAlpha = std::exp(
            -juce::MathConstants<float>::twoPi
            * hpHz
            / static_cast<float>(sampleRate)
        );

        lpCoeff = 1.0f - std::exp(
            -juce::MathConstants<float>::twoPi
            * toneHz
            / static_cast<float>(sampleRate)
        );
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
}
```

## Explicação das partes importantes

```cpp
prepare(double newSampleRate)
```

Chamado quando o plugin recebe sample rate da DAW.

```cpp
reset();
```

Limpa memória interna de filtros.

```cpp
updateFilters();
```

Calcula coeficientes dependentes do sample rate.

```cpp
setDriveDb(float db)
```

Recebe valor em dB, mas guarda em ganho linear, porque áudio usa multiplicação linear.

```cpp
setBias(float newBias)
```

Bias desloca a curva e cria assimetria.

```cpp
jlimit(-0.5f, 0.5f, newBias)
```

Protege contra valores extremos.

```cpp
processSample(float input)
```

Processa uma amostra.

As variáveis intermediárias são intencionais:

```cpp
filteredInput
biasedInput
drivenInput
saturated
dcOffset
corrected
toned
```

Elas tornam o processo legível. Isso é melhor do que:

```cpp
return lowPass(std::tanh((highPass(input) + bias) * driveGain) - std::tanh(bias * driveGain));
```

A versão compacta é esperta, mas ruim para aprender e manter.

---

# 11. `DistortionEngine`: organizando modos de distorção

Arquivo: `Source/DSP/DistortionEngine.h`  
Ação: criar.

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
        bias = newBias;
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
    float bias = 0.0f;

    TubeDistortion tube;
};
}
```

Por que guardar `driveGain`?

Porque isto é ruim dentro do sample:

```cpp
juce::Decibels::decibelsToGain(driveDb)
```

Se você fizer isso a cada amostra, desperdiça CPU.

Melhor converter quando o parâmetro muda:

```cpp
void setDriveDb(float db) noexcept
{
    driveDb = db;
    driveGain = juce::Decibels::decibelsToGain(db);
}
```

---

# 12. Onde o código pode quebrar

## Attachment morrendo

Erro:

```cpp
auto driveAttachment = std::make_unique<SliderAttachment>(...);
```

Correção:

```cpp
std::unique_ptr<SliderAttachment> driveAttachment;
```

O attachment precisa viver enquanto o painel vive.

## Arquivo `.cpp` não registrado no CMake

Criou:

```text
Source/DSP/TubeDistortion.cpp
```

Mas esqueceu o CMake.

Resultado: erro de linker.

## Estado de filtro compartilhado entre canais

Errado:

```cpp
DSP::DistortionEngine engine;
engine.process(left);
engine.process(right);
```

Certo:

```cpp
std::vector<DSP::DistortionEngine> engines;
```

Uma engine por canal.

## Código complexo demais dentro de `processBlock()`

Se `processBlock()` começa a ter muito algoritmo, mova para DSP.

Processor deve orquestrar, não ser um depósito de matemática.

---

# 13. O que vira utilitário, helper ou classe

## Vira utilitário quando:

- não depende de estado;
- recebe valor e retorna valor;
- é usado por mais de um arquivo;
- representa conversão/formatação.

Exemplos:

```cpp
linearToMeterDb()
formatPeakReadout()
gainDbToText()
textToGainDb()
frequencyToText()
percentStringToNormalized()
```

## Vira helper local quando:

- só faz sentido dentro de um arquivo;
- reduz repetição local;
- não merece virar API global.

Exemplo:

```cpp
makeGainParameter(...)
```

## Vira componente quando:

- repete UI;
- tem layout próprio;
- tem comportamento visual próprio.

Exemplo:

```cpp
LabeledSlider
OutputMeter
HeaderBar
```

## Vira classe DSP quando:

- tem estado;
- tem `prepare()`;
- tem `reset()`;
- processa sample ou bloco;
- tem algoritmo próprio.

Exemplo:

```cpp
TubeDistortion
TapeDistortion
DistortionEngine
```

---

# 14. CMake

Sempre que criar `.cpp`, registre:

```cmake
target_sources(FractalDistortion PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp

    Source/DSP/DistortionEngine.cpp
    Source/DSP/TubeDistortion.cpp

    Source/Components/Panels/DistortionPanel/DistortionPanel.cpp
)
```

Headers `.h` normalmente não precisam entrar.

Se uma classe estiver inteira no `.h`, não há `.cpp` para registrar.

---

# 15. Checklist profissional

## Arquitetura

- [ ] UI não faz DSP.
- [ ] DSP não conhece UI.
- [ ] Processor lê parâmetros e chama engine.
- [ ] IDs estão em `ParameterIDs.h`.
- [ ] Conversões estão em `ConversionUtils.h`.
- [ ] Repetição visual virou componente.
- [ ] Engine é uma por canal.

## Didática do código

- [ ] Linhas complexas foram quebradas em variáveis intermediárias.
- [ ] Nomes explicam intenção.
- [ ] Cada função tem uma responsabilidade.
- [ ] Código repetido foi identificado.
- [ ] Comentários explicam o motivo, não só repetem o óbvio.

## Áudio

- [ ] Drive altera volume no primeiro teste.
- [ ] Drive depois altera saturação.
- [ ] Tube tem bias audível.
- [ ] Tone suaviza agudos.
- [ ] Mix 0% é limpo.
- [ ] Mix 100% é processado.
- [ ] Output compensa volume.

## UI

- [ ] Slider aparece.
- [ ] Attachment persiste.
- [ ] Valor salva no preset.
- [ ] Layout não quebra.
- [ ] Componentes comuns não duplicam lógica.

---

# Frase-guia

> Um bom guia de plugin não joga código pronto: ele mostra como o plugin cresce, por que cada parte existe e como testar cada decisão antes de avançar.
