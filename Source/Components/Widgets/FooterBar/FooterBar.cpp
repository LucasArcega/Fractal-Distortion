#include "Components/Widgets/FooterBar/FooterBar.h"

namespace GUI
{
    FooterBar::FooterBar()
    {
        addAndMakeVisible(outputMeterLeft);
        addAndMakeVisible(outputMeterRight);
    }

    void FooterBar::paint(juce::Graphics& g)
    {
        // Sem decoração: a linha separadora é desenhada no editor.
        juce::ignoreUnused(g);
    }

    void FooterBar::resized()
    {
        auto bounds = getLocalBounds();

        // Recuo vertical para respirar.
        bounds.reduce(0, 10);

        const int gap = 6;
        const int rowH = juce::jmax(6, (bounds.getHeight() - gap) / 2);

        // Largura dos medidores: 60% da área útil, centralizado.
        const int meterWidth = juce::roundToInt((float) bounds.getWidth() * 0.6f);
        const int xStart = bounds.getX() + (bounds.getWidth() - meterWidth) / 2;

        int y = bounds.getY();
        outputMeterLeft.setBounds(xStart, y, meterWidth, rowH);
        y += rowH + gap;
        outputMeterRight.setBounds(xStart, y, meterWidth, rowH);
    }
} // namespace GUI