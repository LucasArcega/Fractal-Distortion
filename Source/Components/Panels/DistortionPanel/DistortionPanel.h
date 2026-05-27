#pragma once

#include "Components/LabeledSlider/LabeledSlider.h"
#include "ParameterIDs.h"
#include <JuceHeader.h>

class DistortionPanel : public juce::Component {
public:
    explicit DistortionPanel(juce::AudioProcessorValueTreeState &state)
        : driveControl("Drive"), biasControl("Bias"), toneControl("Tone") {
        addAndMakeVisible(driveControl);
        addAndMakeVisible(biasControl);
        addAndMakeVisible(toneControl);
        // Conectar ao parâmetro
        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, ParameterIDs::driveDb.getParamID(), driveControl.getSlider());

        biasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, ParameterIDs::bias.getParamID(), biasControl.getSlider());

        toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            state, ParameterIDs::toneHz.getParamID(), toneControl.getSlider());

        typeCombo.addItem("Tube", 1);
        typeCombo.addItem("Soft Clip", 2);
        typeCombo.addItem("Hard Clip", 3);
        addAndMakeVisible(typeCombo);

        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            state, ParameterIDs::distortionType.getParamID(), typeCombo);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(24);

        auto defaltMargin = FlexItem::Margin(0.f, 0.f, 10.f, 0.f); // Margem padrão para os itens

        // Define A row Others, que contem bias e tone.
        juce::FlexBox rowOthers;
        rowOthers.flexDirection = juce::FlexBox::Direction::row;
        rowOthers.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
        rowOthers.items.add(juce::FlexItem(biasControl).withFlex(1.f));
        rowOthers.items.add(juce::FlexItem(toneControl).withFlex(1.f));

        // Define a flexBox Principal, em column layout, onde o primeiro item é o comboBox, o
        // segundo é o driveControl e o terceiro é a rowOthers (bias e tone)
        juce::FlexBox mainBox;
        mainBox.flexDirection = juce::FlexBox::Direction::column;
        mainBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
        mainBox.items.add(
            juce::FlexItem(typeCombo).withFlex(0.25f).withMaxHeight(24.f).withMargin(defaltMargin));
        mainBox.items.add(juce::FlexItem(driveControl).withFlex(3.f).withMargin(defaltMargin));
        mainBox.items.add(juce::FlexItem(rowOthers).withFlex(.75f).withMargin(defaltMargin));

        mainBox.performLayout(area.toFloat());
    }

private:
    Common::LabeledSlider driveControl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

    Common::LabeledSlider biasControl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> biasAttachment;

    Common::LabeledSlider toneControl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;

    juce::ComboBox typeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
};