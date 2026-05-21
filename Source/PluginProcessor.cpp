#include "PluginProcessor.h"
#include "PluginEditor.h"


// Reponsável por inicializar os parametros do plugin, definindo seus intervalos, valores padrão e
// nomes. Esses parâmetros serão usados para controlar o comportamento do plugin e serão expostos na
// interface do usuário.
juce::AudioProcessorValueTreeState::ParameterLayout FractalDistortionAudioProcessor::createParameterLayout()
{
    const auto range = juce::NormalisableRange<float>(-60.f, 12.f, 0.1f, 0.45f);
    const float defaultGainValue = 0.f;
    const auto driveNormalizedRange = juce::NormalisableRange<float>(0.0f, 36.0f, 0.1f, 0.45f);
    const float driveStartValue = 2.f;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"inputGainDb", 1},
                                                           "Input", range, defaultGainValue));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"outputGainDb", 1},
                                                           "Output", range, defaultGainValue));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::driveDb, "Drive",
                                                           driveNormalizedRange, driveStartValue));
    return layout;
}

FractalDistortionAudioProcessor::FractalDistortionAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

void FractalDistortionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

bool FractalDistortionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();
    if (out != in) return false;
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

// Aqui processamos o audio, aplicando os efeitos e alterações necessárias de acordo com os
// paramentros definidos. O ganho de entrada e saída é aplicado ao buffer de áudio, e o
// processamento de distorção pode ser adicionado posteriormente usando o valor do parâmetro
// "driveDb".
void FractalDistortionAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    juce::ScopedNoDenormals noDenormals;

    const float driveGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("driveDb")->load());
    const float inGain  = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("inputGainDb")->load());
    const float outGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGainDb")->load());

    buffer.applyGain(inGain);
    buffer.applyGain(outGain);

    //Aplicar drive simples, transformar em método depois:
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto *samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            samples[sample] *= std::tanh(samples[sample]* driveGain);
        }
    }
}

juce::AudioProcessorEditor* FractalDistortionAudioProcessor::createEditor()
{
    return new FractalDistortionAudioProcessorEditor(*this);
}

void FractalDistortionAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void FractalDistortionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FractalDistortionAudioProcessor();
}
