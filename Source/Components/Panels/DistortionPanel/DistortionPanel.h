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

        typeCombo.addItem("Tube", 1);
        typeCombo.addItem("Soft Clip", 2);
        typeCombo.addItem("Hard Clip", 3);
        addAndMakeVisible(typeCombo);

        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            state, ParameterIDs::distortionType.getParamID(), typeCombo);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);
 
        typeCombo.setBounds(area.removeFromTop(28)); // Combo box no topo
        area.removeFromTop(24); // Espaço para o combo box
        driveControl.setBounds(area);
    }

private:
    Common::LabeledSlider driveControl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

    juce::ComboBox typeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
};