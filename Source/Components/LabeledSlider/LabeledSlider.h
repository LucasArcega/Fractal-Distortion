#pragma once

#include <JuceHeader.h>
#include "Components/Widgets/StyleUtils.h"

namespace Common {
    class LabeledSlider : public juce::Component {
    public:
        explicit LabeledSlider(const juce::String& labelText,
                               juce::uint32 accentColour = GUI::Colors::AccentOrange)
        {
            GUI::styleParameterLabel(label, labelText, GUI::Colors::TextPrimary, GUI::FontSizes::ParameterLabel,
                                     juce::Justification::centred);
            addAndMakeVisible(label);

            GUI::styleRotaryKnob(slider, accentColour, true);
            addAndMakeVisible(slider);
        }

        juce::Slider& getSlider() noexcept { return slider; }

        void resized() override
        {
            juce::FlexBox layout;
            layout.flexDirection = juce::FlexBox::Direction::column;
            layout.justifyContent = juce::FlexBox::JustifyContent::flexStart;
            layout.alignItems = juce::FlexBox::AlignItems::stretch;
            layout.items.add(juce::FlexItem(label).withHeight(14.f));
            layout.items.add(juce::FlexItem(slider).withFlex(1.f).withMinHeight(72.f));
            layout.performLayout(getLocalBounds().toFloat());
        }

    private:
        juce::Label label;
        juce::Slider slider;
    };
} // namespace Common
