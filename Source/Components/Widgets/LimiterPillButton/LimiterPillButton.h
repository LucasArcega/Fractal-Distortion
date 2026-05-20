#pragma once

#include <JuceHeader.h>

namespace GUI
{

/** Toggle estilo “pill” do mockup: cantos completamente redondos, contorno roxo, texto ON/OFF em accent quando activo. */
class LimiterPillButton : public juce::Button
{
public:
    LimiterPillButton();

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterPillButton)
};

} // namespace GUI
