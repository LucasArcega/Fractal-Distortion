#pragma once

#include <JuceHeader.h>
#include <Utils/ConversionUtils.h>

/// <summary>
/// Component que representa um painel de controle de ganho (input ou output).
/// </summary>
class GainPanel : public juce::Component {
public:
    explicit GainPanel(juce::AudioProcessorValueTreeState &state,
                       const juce::String &columnTitle,
                       const juce::String &peakLabel,
                       const juce::String &parameterID) {

        GUI::styleColumnTitle(gainTitle, columnTitle);
        GUI::stylePeakLabel(gainLabel, peakLabel, GUI::Colors::TextMuted, 12.f);
        GUI::styleLinearSlider(gainSlider);
        GUI::stylePeakLabel(maxPeakLabel, "Max", GUI::Colors::TextMuted, 12.f);

        //  Habilita tooltips;
        gainLabel.setTooltip("Current peak level in dBFS");
        maxPeakLabel.setTooltip("Maximum peak level in dBFS");

        maxPeakLabel.addMouseListener(this, false);
        maxPeakLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        gainLabel.addMouseListener(this, false);
        gainLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);

        addAndMakeVisible(gainTitle);
        addAndMakeVisible(gainLabel);
        addAndMakeVisible(maxPeakLabel);
        addAndMakeVisible(gainSlider);

        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, parameterID, gainSlider);
    }

    void mouseDown(const juce::MouseEvent &event) override 
    {
        if (event.eventComponent == &gainLabel ||
            event.eventComponent == &maxPeakLabel)
            this->resetPeak();
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
        fb.items.add(juce::FlexItem(maxPeakLabel).withHeight(peakH));
        
        fb.items.add(juce::FlexItem(gainLabel).withHeight(peakH));
        fb.items.add(juce::FlexItem(gainSlider)
                         .withFlex(1.f)
                         .withMinHeight(sliderMinH)
                         .withMaxWidth((float)getWidth()));

        fb.performLayout(r);
    }

    void updatePeakLabel(float linearPeak) {

        // Usado para decaimento do valor de pico, para evitar que o valor fique oscilando muito
        // rapidamente e seja difícil de ler.
        constexpr float decayFactor = 0.85f;
        currentPeak = juce::jmax(linearPeak, currentPeak * decayFactor);
        maxPeak = juce::jmax(maxPeak, currentPeak);
        
        const juce::String text = fractal_utils::formatPeakReadout(currentPeak);
        gainLabel.setText(text, juce::dontSendNotification);

        // valor máximo é atualizado apenas para leitura, não tem decaimento, para que o usuário
        // possa ler o valor máximo mesmo que o pico atual já tenha caído.
        const juce::String maxText = fractal_utils::formatPeakReadout(maxPeak);
        maxPeakLabel.setText(maxText, juce::dontSendNotification);
    }

    void resetPeak() {
        currentPeak = 0.f;
        maxPeak = 0.f;
        gainLabel.setText("-inf", juce::dontSendNotification);
        maxPeakLabel.setText("-inf", juce::dontSendNotification);
    }

private:
    juce::Label gainTitle;
    juce::Label gainLabel;
    juce::Label maxPeakLabel;
    juce::Slider gainSlider;

    float maxPeak = 0.f;
    float currentPeak = 0.f;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainPanel)
};