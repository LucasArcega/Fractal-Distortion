#include "Components/Widgets/DbScaleRuler/DbScaleRuler.h"

namespace GUI
{
void DbScaleRuler::paint(juce::Graphics& g)
{
    auto rulerBounds = getLocalBounds().toFloat();
    // Mesma faixa de 14 px que `VerticalSegmentMeter` reserva para "IN"/"OUT", para os ticks alinharem à escada LED.
    rulerBounds.removeFromTop(14.f);
    g.setFont(juce::Font(juce::FontOptions(9.f)));
    g.setColour(juce::Colour(0xff9aa7b8));

    static constexpr float tickLevelsDb[] = { 0.f, -6.f, -12.f, -18.f, -24.f, -36.f, -48.f, -60.f };

    for (float tickDb : tickLevelsDb)
    {
        const float tickCentreY = juce::jmap(tickDb, -60.f, 6.f, rulerBounds.getBottom(), rulerBounds.getY());
        const juce::String tickLabel = (tickDb == 0.f) ? juce::String("0") : juce::String((int) tickDb);
        g.drawText(tickLabel,
                   juce::Rectangle<float>(rulerBounds.getX(), tickCentreY - 5.f, rulerBounds.getWidth(), 10.f),
                   juce::Justification::centredRight,
                   false);
    }
}

} // namespace GUI
