#pragma once

#include <JuceHeader.h>

#include "Components/Panels/OutputPanel/OutputPanel.h"

#include "Components/Widgets/FooterBar/FooterBar.h"
#include "PluginProcessor.h"

/**
 * @brief Interface gráfica principal do plugin Fractal-Delay.
 *
 * @details Estrutura de layout (960×684 pixels):
 *
 * ┌─────────────────────────────────────────────────┐
 * │ HeaderStrip (44px)                              │ ← Marca, tabs, preset, BPM
 * ├─────────────┬──────────────────┬────────────────┤
 * │   IN        │  TapEditor       │     OUT        │
 * │  Column     │  (center)        │   Column       │ ← Grid 3 colunas (Fr: 100:145:100)
 * │             │                  │                │
 * ├─────────────┴──────────────────┴────────────────┤
 * │ Bottom Strip (236px) = 3 painéis                │
 * │ ┌──────────┬───────────┬─────────────────────┐ │
 * │ │ Delay    │Character  │ Output              │ │ ← DelayCorePanel | CharacterPanel | OutputPanel
 * │ │ Core     │           │                     │ │
 * │ └──────────┴───────────┴─────────────────────┘ │
 * ├─────────────────────────────────────────────────┤
 * │ FooterBar (52px) - Medidores estéreo L/R       │
 * └─────────────────────────────────────────────────┘
 *
 * @section layout_system Sistema de Layout
 *
 * - **Macro layout**: juce::Grid (3 rows × 3 cols para a área central)
 * - **Micro layout**: juce::FlexBox dentro de cada componente
 *
 * @section metering_system Sistema de Metering (Padrão Airwindows)
 *
 * **CRÍTICO PARA WINDOWS**: Para evitar RAM explosion, o editor usa um padrão
 * assíncrono de comunicação com a thread de áudio:
 *
 * 1. **IdleTimer** (~30Hz) chama idle() periodicamente
 * 2. **idle()** drena a fila lock-free de mensagens (audioToUI)
 * 3. **Atualiza UI** apenas quando valores mudam (evita repaint desnecessário)
 * 4. **repaint()** é chamado SOMENTE se necessário (não incondicionalmente!)
 *
 * @warning NUNCA use Timer::startTimer() no construtor com editor não "showing".
 *          No Standalone, o editor pode não estar visível no primeiro frame.
 *          IdleTimer só é iniciado APÓS addAndMakeVisible() de todos os componentes.
 *
 * @see PluginProcessor (comunicação audio→UI)
 * @see Plans/00-convencoes-repo-ui-testes.md (padrão Airwindows)
 */
class FractalDistortionAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit FractalDistortionAudioProcessorEditor(FractalDistortionAudioProcessor&);
    ~FractalDistortionAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    /**
     * @brief Drena a fila de mensagens audio→UI e atualiza medidores.
     *
     * @details Chamado pelo IdleTimer a ~30Hz. Fluxo:
     * 1. Loop enquanto houver mensagens na fila (audioToUI.pop)
     * 2. Atualiza variáveis locais (currentPeakIn, currentPeakOut, etc.)
     * 3. Quando recebe INCREMENT, atualiza visualmente:
     *    - Labels de texto (IN: X dB, OUT: Y dB)
     *    - FooterBar medidores (L/R)
     *    - OutputPanel medidores (IN/OUT)
     *
     * @warning Este método NUNCA deve bloquear ou alocar memória pesada.
     *          É chamado na message thread, mas precisa ser rápido.
     *
     * @see PluginProcessor::processBlock (quem empurra mensagens)
     */
    void idle();

    /**
     * @brief Timer auxiliar que chama idle() periodicamente (~30Hz).
     *
     * @details Padrão Airwindows: usar um timer separado (não herdar de Timer
     * diretamente no editor) permite maior flexibilidade e clareza.
     *
     * O timer é iniciado APÓS todos os componentes serem criados (addAndMakeVisible),
     * garantindo que a UI está pronta antes de começar a drenar mensagens.
     *
     * @note Frequência de ~30Hz (33ms) é suficiente para atualização visual suave
     *       sem sobrecarregar a message thread.
     */
    struct IdleTimer : juce::Timer
    {
        explicit IdleTimer(FractalDistortionAudioProcessorEditor* e) : ed(e) {}
        void timerCallback() override { ed->idle(); }
        FractalDistortionAudioProcessorEditor* ed;
    };

private:
    /** @brief Referência ao processador (acessado para ler APVTS e drenar fila audioToUI). */
    FractalDistortionAudioProcessor& audioProcessor;

    /** @brief Timer que chama idle() a ~30Hz para drenar mensagens audio→UI. */
    std::unique_ptr<IdleTimer> idleTimer;

    // ========== Componentes de Chrome (Header + Footer) ==========
    GUI::FooterBar footerBar;      ///< Rodapé com medidores estéreo L/R

    // ========== Colunas da Grid Central (IN | CENTER | OUT) ==========
    juce::Component inColumn;      ///< Coluna esquerda: título IN + gain slider vertical
    juce::Component centerColumn;  ///< Coluna central: TapEditor (grelha visual)
    juce::Component outColumn;     ///< Coluna direita: título OUT + label de pico

    // ========== Widgets da Coluna IN ==========
    juce::Label inTitle;           ///< Título "IN"
    juce::Label inLabel;           ///< Label "IN: X dB" (atualizado em idle)
    juce::Slider inputSlider;      ///< Slider vertical de ganho de entrada

    // ========== Widget da Coluna CENTER ==========

    // ========== Widgets da Coluna OUT ==========
    juce::Label outTitle;          ///< Título "OUT"
    juce::Label outLabel;          ///< Label "OUT: X dB" (atualizado em idle)

    // ========== Painéis Inferiores (Bottom Strip) ==========

    GUI::OutputPanel outputPanel;          ///< Painel OUTPUT (mix, gain, limiter, meters)

    // ========== APVTS Attachments ==========
    /** @brief Attachment do slider de ganho de entrada ao parâmetro "inputGainDb". */
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment;

    // ========== Cache de Picos (atualizados em idle()) ==========
    float currentPeakIn        = 0.f;  ///< Último pico de entrada recebido (linear)
    float currentPeakOut       = 0.f;  ///< Último pico de Output geral (max L/R, linear)
    float currentPeakOutLeft   = 0.f;  ///< Último pico do canal esquerdo (linear)
    float currentPeakOutRight  = 0.f;  ///< Último pico do canal direito (linear)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FractalDistortionAudioProcessorEditor)
};
