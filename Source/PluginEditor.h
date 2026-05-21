#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Components/Panels/OutputPanel/OutputPanel.h"
#include "Components/Panels/DistortionPanel/DistortionPanel.h"
#include "Components/Widgets/FooterBar/FooterBar.h"
#include "Components/Widgets/StyleUtils.h"

/* Header do nosso editor. Equivalente a parte visual principal do programa*/

class FractalDistortionAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit FractalDistortionAudioProcessorEditor(FractalDistortionAudioProcessor&);
    ~FractalDistortionAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void idle();

private:
    /** @brief Timer que drena a fila audio?UI a ~30Hz. */
    struct IdleTimer : juce::Timer
    {
        explicit IdleTimer(FractalDistortionAudioProcessorEditor* e) : editor(e) {}
        void timerCallback() override { editor->idle(); }
        FractalDistortionAudioProcessorEditor* editor;
    };

    FractalDistortionAudioProcessor& audioProcessor;

    GUI::FooterBar   footerBar;
    GUI::OutputPanel outputPanel;
    DistortionPanel  distortionPanel;

    juce::Component inColumn;
    juce::Component centerColumn;
    juce::Component outColumn;

    juce::Label  inTitle;
    juce::Label  inLabel;
    juce::Slider inputSlider;

    juce::Label outTitle;
    juce::Label outLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment;

    std::unique_ptr<IdleTimer> idleTimer;

    float currentPeakIn       = 0.f;
    float currentPeakOut      = 0.f;
    float currentPeakOutLeft  = 0.f;
    float currentPeakOutRight = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FractalDistortionAudioProcessorEditor)
};
