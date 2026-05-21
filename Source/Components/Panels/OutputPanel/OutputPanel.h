#pragma once

#include <JuceHeader.h>

#include "Components/Widgets/DbScaleRuler/DbScaleRuler.h"
#include "Components/Widgets/LimiterPillButton/LimiterPillButton.h"
#include "Components/Widgets/VerticalSegmentMeter/VerticalSegmentMeter.h"

#include <memory>

class FractalDistortionAudioProcessor;

namespace GUI
{

/**
 * @brief Painel OUTPUT: controles de mix, ganho, limiter e metering.
 *
 * @details Layout hierárquico (Grid + FlexBox):
 *
 * ┌────────────────────────────────────────────┐
 * │ OUTPUT (título)                            │ ← sectionTitle
 * ├───────────────────────────┬────────────────┤
 * │ ControlsRow               │ MeterCluster   │ ← Grid 2 colunas (Fr: 100:48)
 * │ ┌────────┬──────────────┐ │ ┌──┬──┬─────┐ │
 * │ │ Mix    │ Gain+Limiter │ │ │IN│OUT│Scale││
 * │ │ Column │  Column      │ │ └──┴──┴─────┘ │
 * │ └────────┴──────────────┘ │ ┌──┬──┐       │
 * │                           │ │-∞│-∞│       │ ← Peak readouts
 * │                           │ └──┴──┘       │
 * └───────────────────────────┴────────────────┘
 *
 * @section nested_components Componentes Aninhados
 *
 * - **ControlsRow**: Contêiner horizontal para MixColumn + GainLimiterColumn
 *   - **MixColumn**: Label "MIX" + knob rotativo (0…100%)
 *   - **GainLimiterColumn**: Label "GAIN" + knob rotativo (-60…+12 dB) + Label "LIMITER" + toggle pill
 * - **MeterCluster**: Contêiner para medidores verticais + escala dB
 *   - **VerticalSegmentMeter** (IN): Medidor vertical segmentado (estilo LED)
 *   - **VerticalSegmentMeter** (OUT): Medidor vertical segmentado
 *   - **DbScaleRuler**: Régua vertical de escala (-60…+6 dB)
 *   - **Peak readouts**: Labels "-inf" ou "X.X dB" abaixo dos medidores
 *
 * @note Structs aninhadas são específicas deste painel (não reutilizáveis).
 *       Se outras classes precisarem delas, considerar extrair para arquivos próprios.
 *
 * @see Plans/fase-1-motor-delay-basico/04-metering-visual.md
 */
class OutputPanel : public juce::Component
{
public:
    explicit OutputPanel(FractalDistortionAudioProcessor&);

    void resized() override;
    void paint(juce::Graphics& g) override;

    /**
     * @brief Atualiza medidores visuais com novos picos de Input/Output.
     *
     * @details Chamado por PluginEditor::idle() quando recebe mensagens audio→UI.
     * Converte amplitudes lineares para dB e atualiza MeterCluster.
     *
     * @param peakInLinear  Pico de Input (linear, 0…∞)
     * @param peakOutLinear Pico de Output geral (max L/R, linear, 0…∞)
     */
    void updateOutputMetering(float peakInLinear, float peakOutLinear);

private:
    /** @brief Aplica estilo padrão a um knob rotativo. */
    static void styleRotary(juce::Slider& s, bool interactive);

    /** @brief Referência ao processador (acesso ao APVTS). */
    FractalDistortionAudioProcessor& audioProcessor;

    /**
     * @brief Coluna MIX: label + knob rotativo (0…100% wet).
     *
     * @details Layout vertical FlexBox: label fixo (14px) + knob flexível.
     */
    struct MixColumn final : public juce::Component
    {
        juce::Label mixLabel;   ///< "MIX"
        juce::Slider mixKnob;   ///< Knob rotativo (parâmetro "delayMix")
        MixColumn();
        void resized() override;
    };

    /**
     * @brief Coluna GAIN + LIMITER: label + knob + label + toggle pill.
     *
     * @details Layout vertical FlexBox:
     * - gainLabel (14px)
     * - gainKnob (flexível)
     * - limiterLabel (10px)
     * - limiterPill (30px, max width 84px, centrado)
     */
    struct GainLimiterColumn final : public juce::Component
    {
        juce::Label gainLabel;         ///< "GAIN"
        juce::Slider gainKnob;         ///< Knob rotativo (parâmetro "outputGainDb")
        juce::Label limiterLabel;      ///< "LIMITER"
        LimiterPillButton limiterPill; ///< Toggle pill (não funcional Fase 1)
        GainLimiterColumn();
        void resized() override;
    };

    /**
     * @brief Contêiner horizontal para MixColumn + GainLimiterColumn.
     *
     * @details Layout horizontal FlexBox:
     * - MixColumn (flex 1, min 48px)
     * - Gap de 8px
     * - GainLimiterColumn (preferred 112px, min 72px, max 112px, shrink 1)
     */
    struct ControlsRow final : public juce::Component
    {
        MixColumn mix;             ///< Coluna MIX
        GainLimiterColumn gain;    ///< Coluna GAIN + LIMITER
        ControlsRow();
        void resized() override;
    };

    /**
     * @brief Cluster de medidores: IN + OUT + escala dB + peak readouts.
     *
     * @details Layout:
     * - Fila superior: 2 medidores verticais (40px cada) + régua (24px)
     * - Fila inferior (16px): labels de pico "-inf" ou "X.X dB"
     */
    struct MeterCluster final : public juce::Component
    {
        VerticalSegmentMeter inMeter { "IN" };   ///< Medidor vertical IN
        VerticalSegmentMeter outMeter { "OUT" }; ///< Medidor vertical OUT
        DbScaleRuler dbScaleRuler;               ///< Régua de escala dB
        juce::Label inPeakReadout;               ///< Pico IN (texto)
        juce::Label outPeakReadout;              ///< Pico OUT (texto)
        MeterCluster();
        void resized() override;

        /**
         * @brief Atualiza níveis dos medidores e labels de pico.
         *
         * @param peakInLinear  Amplitude de Input (linear)
         * @param peakOutLinear Amplitude de Output (linear)
         */
        void updateLevels(float peakInLinear, float peakOutLinear);
    };

    juce::Label sectionTitle;
    ControlsRow controls;
    MeterCluster meters;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixKnobAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainKnobAttachment;

    /** X em coords. do painel para a linha entre controlos e medidores; -1 = não desenhar. Actualizado em `resized()` após o Grid. */
    int meterDividerX { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputPanel)
};

} // namespace GUI
