#include "Components/Widgets/VerticalSegmentMeter/VerticalSegmentMeter.h"

#include <cmath>

namespace GUI
{
namespace
{
    /** @brief Cor dos LEDs acesos (roxo/lavanda). */
    constexpr juce::uint32 kLitSegmentColour = 0xffb388ff;

    /** @brief Cor dos LEDs apagados (cinza escuro). */
    constexpr juce::uint32 kDimSegmentColour = 0xff2a2f3a;

    /**
     * @brief Número de segmentos LED verticais.
     *
     * @details 22 LEDs dividem a faixa de 66 dB (-60…+6) linearmente.
     * Cada LED representa ~3 dB.
     */
    constexpr int kNumLedSegments = 22;

    /**
     * @brief Altura reservada para o label do canal (topo).
     *
     * @note Deve ser consistente com DbScaleRuler e MeterCluster layout.
     */
    constexpr float kChannelTitleBandHeightPx = 14.f;
} // namespace

VerticalSegmentMeter::VerticalSegmentMeter(juce::String channelLabelTextIn)
    : channelLabelText(std::move(channelLabelTextIn))
{
}

/**
 * @brief Desenha o medidor vertical segmentado.
 *
 * @details Ordem de rendering:
 * 1. **Label do canal** (topo, 14px, branco, centrado)
 * 2. **Frame** (fundo escuro + borda sutil + cantos redondos)
 * 3. **22 LEDs** (de baixo para cima, lit ou dim conforme nível)
 *
 * @section led_algorithm Algoritmo de Iluminação
 *
 * 1. Normaliza level: (levelDb + 60) / 66 → [0, 1]
 * 2. Calcula LEDs acesos: normalized × 22
 * 3. Para cada LED (bottom=0 … top=21):
 *    - Se index+0.5 < numSegmentsLit: aceso (lit)
 *    - Caso contrário: apagado (dim)
 *
 * O offset de 0.5 garante que a transição ocorra no meio do LED.
 */
void VerticalSegmentMeter::paint(juce::Graphics& g)
{
    auto fullBounds = getLocalBounds().toFloat();

    // 1. Desenha label do canal (topo)
    auto channelTitleBand = fullBounds.removeFromTop(kChannelTitleBandHeightPx);
    g.setFont(juce::Font(juce::FontOptions(10.f)));
    g.setColour(juce::Colour(0xffe8eaed));
    g.drawText(channelLabelText, channelTitleBand, juce::Justification::centred);

    // 2. Desenha frame do medidor (fundo + borda)
    constexpr float kMeterFrameCornerRadius = 4.f;
    g.setColour(juce::Colour(0xff12151c));  // Fundo escuro
    g.fillRoundedRectangle(fullBounds, kMeterFrameCornerRadius);
    g.setColour(juce::Colour(0xff3d4450));  // Borda sutil
    g.drawRoundedRectangle(fullBounds, 1.f, 1.f);

    // 3. Desenha 22 segmentos LED (de baixo para cima)
    auto segmentStackArea = fullBounds.reduced(3.f, 3.f);
    const float segmentStackHeightPx = juce::jmax(1.f, segmentStackArea.getHeight());
    const float segmentHeightPx = segmentStackHeightPx / (float) kNumLedSegments;

    // Normaliza nível: -60 dB = 0, +6 dB = 1 (66 dB de range)
    const float normalizedLevel01 = juce::jlimit(0.f, 1.f, (levelDb + 60.f) / 66.f);
    const float numSegmentsLit = normalizedLevel01 * (float) kNumLedSegments;

    for (int segmentIndex = 0; segmentIndex < kNumLedSegments; ++segmentIndex)
    {
        // Desenha de baixo para cima (index 0 = bottom = -60 dB)
        const float segmentTopY = segmentStackArea.getBottom() - (float) (segmentIndex + 1) * segmentHeightPx;
        auto segmentRect = segmentStackArea.withY(segmentTopY).withHeight(juce::jmax(1.f, segmentHeightPx - 1.f));

        // LED aceso se index+0.5 < numSegmentsLit (transição no meio do LED)
        const bool segmentIsLit = ((float) segmentIndex + 0.5f) < numSegmentsLit;

        // Cor: roxo/lavanda (lit) ou cinza escuro (dim)
        g.setColour(segmentIsLit ? juce::Colour(kLitSegmentColour).withAlpha(0.92f) : juce::Colour(kDimSegmentColour));
        g.fillRoundedRectangle(segmentRect.reduced(2.f, 0.f), 2.f);
    }
}

    /**
    * @brief Atualiza o nível do medidor (com threshold de repaint).
    *
    * @details Otimização: só repinta se o valor mudar > 0.05 dB.
    * Threshold maior que HorizontalMetter (0.05 vs 0.02) porque medidores
    * segmentados têm resolução mais baixa (~3 dB por LED).
    */
    void VerticalSegmentMeter::setLevelDb(float newDb) noexcept
    {
        const float clampedDb = juce::jlimit(-60.f, 6.f, newDb);
        if (std::abs(clampedDb - levelDb) < 0.05f)
            return;  // Mudança < 0.05 dB é imperceptível visualmente
        levelDb = clampedDb;
        repaint();
    }

} // namespace GUI
