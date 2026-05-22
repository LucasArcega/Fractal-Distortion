#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Components/Panels/OutputPanel/OutputPanel.h"
#include "Components/Panels/DistortionPanel/DistortionPanel.h"
#include "Components/Widgets/FooterBar/FooterBar.h"
#include "Components/Widgets/StyleUtils.h"
#include "Components/Panels/GainPanel/GainPanel.h"

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
    
    GainPanel        inputGainPanel;
    GainPanel        outputGainPanel;

    juce::Component inColumn;
    juce::Component centerColumn;
    juce::Component outColumn;

    std::unique_ptr<IdleTimer> idleTimer;

    float currentPeakOutLeft  = 0.f;
    float currentPeakOutRight = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FractalDistortionAudioProcessorEditor)
};
