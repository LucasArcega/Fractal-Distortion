#include "Components/Widgets/LimiterPillButton/LimiterPillButton.h"

namespace GUI
{
namespace
{
    constexpr juce::uint32 colAccent = 0xffb388ff;
    constexpr juce::uint32 colFillOff = 0xff12151c;
    constexpr juce::uint32 colFillOn = 0xff22182e;
    constexpr juce::uint32 colTextMuted = 0xff9aa7b8;
} // namespace

LimiterPillButton::LimiterPillButton()
    : juce::Button(juce::String())
{
    setClickingTogglesState(true);
    setToggleState(true, juce::dontSendNotification);
}

void LimiterPillButton::paintButton(juce::Graphics& g,
                                    bool shouldDrawButtonAsHighlighted,
                                    bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f, 1.f);
    const float rad = bounds.getHeight() * 0.5f;
    const bool on = getToggleState();
    const bool pressed = shouldDrawButtonAsDown;

    const auto fill = on ? juce::Colour(colFillOn) : juce::Colour(colFillOff);
    const auto border = juce::Colour(colAccent).withAlpha(on ? 0.95f : 0.55f);
    const auto textCol = on ? juce::Colour(colAccent) : juce::Colour(colTextMuted);

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, rad);

    if (pressed)
    {
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.fillRoundedRectangle(bounds, rad);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.fillRoundedRectangle(bounds, rad);
    }

    g.setColour(border);
    g.drawRoundedRectangle(bounds, rad, on ? 1.6f : 1.2f);

    g.setFont(juce::Font(juce::FontOptions(11.f)).boldened());
    g.setColour(textCol);
    g.drawText(on ? "ON" : "OFF", bounds, juce::Justification::centred, false);
}

} // namespace GUI
