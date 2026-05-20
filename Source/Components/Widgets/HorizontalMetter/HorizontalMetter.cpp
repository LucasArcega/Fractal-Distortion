#include "Components/Widgets/HorizontalMetter/HorizontalMetter.h"

#include <cmath>

namespace GUI
{
    /**
     * @brief Desenha o medidor horizontal.
     *
     * @details Ordem de rendering:
     * 1. Fundo escuro (InputBackground)
     * 2. Borda sutil (Border, 1px)
     * 3. Barra de nível com gradiente (se level > -60 dB)
     *
     * @section gradient Gradiente de Cor
     *
     * A barra interpola entre dois tons de roxo/lavanda:
     * - -60 dB: 0xff6a4dcc (roxo escuro)
     * - +6 dB: 0xffe8b4ff (lavanda claro)
     *
     * Mapeamento LINEAR visual: 0px (-60 dB) … width (+6 dB)
     */
    void HorizontalMetter::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const float rad = juce::jmin(3.f, bounds.getHeight() * 0.45f);

        // Fundo escuro
        g.setColour(juce::Colour(0xff12151c));
        g.fillRoundedRectangle(bounds, rad);

        // Borda sutil
        g.setColour(juce::Colour(0xff3d4450));
        g.drawRoundedRectangle(bounds, rad, 1.f);

        // Barra de nível com gradiente
        const float scaledW = juce::jmap(level, -60.f, 6.f, 0.f, (float) getWidth());
        if (scaledW > 0.f)
        {
            auto filled = bounds.reduced(1.f).withWidth(scaledW);
            // Interpolação de cor: t=0 (roxo escuro) → t=1 (lavanda claro)
            const float t = juce::jlimit(0.f, 1.f, (level + 60.f) / 66.f);
            auto barColour = juce::Colour(0xff6a4dcc).interpolatedWith(juce::Colour(0xffe8b4ff), t);
            g.setColour(barColour);
            g.fillRoundedRectangle(filled, rad - 0.5f);
        }
    }

    /**
     * @brief Atualiza o nível do medidor (com threshold de repaint).
     *
     * @details Otimização: só repinta se o valor mudar > 0.02 dB.
     * Isso evita centenas de repaints desnecessários quando o nível
     * oscila levemente (ruído digital, quantização, etc.).
     */
    void HorizontalMetter::setLevel(float levelDb) noexcept
    {
        // Threshold de 0.02 dB é imperceptível visualmente mas poupa muitos repaints
        if (std::fabs((double) level - (double) levelDb) < 0.02)
            return;
        level = levelDb;
        repaint();
    }
} // namespace GUI
