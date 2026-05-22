#pragma once

#include <JuceHeader.h>

/// <summary>
/// Component que representa um painel de controle de ganho (input ou output).
/// </summary>
class GainPanel : public juce::Component {
public:
    explicit GainPanel(juce::AudioProcessorValueTreeState &state, const juce::String &columnTitle,
                       const juce::String &peakLabel, const juce::String &parameterID) {

        GUI::styleColumnTitle(gainTitle, columnTitle);
        GUI::stylePeakLabel(gainLabel, peakLabel, GUI::Colors::TextMuted, 12.f);
        GUI::styleLinearSlider(gainSlider);

        addAndMakeVisible(gainTitle);
        addAndMakeVisible(gainLabel);
        addAndMakeVisible(gainSlider);

        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, parameterID, gainSlider);
    }

    void resized() override {

        auto r = getLocalBounds().toFloat();
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::column;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        fb.alignItems = juce::FlexBox::AlignItems::stretch;

        constexpr float titleH = 20.f;
        constexpr float peakH = 22.f;
        constexpr float sliderMinH = 120.f;

        fb.items.add(juce::FlexItem(gainTitle).withHeight(titleH));
        fb.items.add(juce::FlexItem(gainLabel).withHeight(peakH));
        fb.items.add(juce::FlexItem(gainSlider)
                         .withFlex(1.f)
                         .withMinHeight(sliderMinH)
                         .withMaxWidth((float)getWidth()));

        fb.performLayout(r);
    }

private:
    juce::Label gainTitle;
    juce::Label gainLabel;
    juce::Slider gainSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainPanel)
};