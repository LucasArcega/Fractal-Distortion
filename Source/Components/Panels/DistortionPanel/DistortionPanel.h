#pragma once

#include <JuceHeader.h>
#include "ParameterIDs.h"
#include "Components/LabeledSlider/LabeledSlider.h"


class DistortionPanel : public juce::Component 
{
public:
    explicit DistortionPanel(juce::AudioProcessorValueTreeState& state)
        : driveControl("Drive")
    {
        addAndMakeVisible(driveControl);
        // Conectar ao parâmetro
        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, ParameterIDs::driveDb.getParamID(), driveControl.getSlider());
    }

    void resized() override
    {
        driveControl.setBounds(getLocalBounds().reduced(12));
    }

private:
    Common::LabeledSlider driveControl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
};