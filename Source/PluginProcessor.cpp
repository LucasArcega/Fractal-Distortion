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
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::inputGainDb,
                                                           "Input", range, defaultGainValue));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParameterIDs::outputGainDb,
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

/// <summary>
/// Basicamente a coração do processamento do audio. Aqui é onde a mágica acontece. O método
/// processBlock é chamado para cada bloco de áudio que precisa ser processado. Ele recebe um buffer
/// de áudio e um buffer de MIDI (que não estamos usando neste caso). Dentro deste método, aplicamos
/// o ganho de entrada, a distorção e o ganho de saída ao buffer de áudio. Também atualizamos os
/// picos de entrada e saída para monitoramento.
/// </summary>
/// <param name="buffer"></param>
/// <param name="midi"></param>
void FractalDistortionAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    juce::ScopedNoDenormals noDenormals;

    // Obter os valores de ganho dos parâmetros, convertendo de dB para ganho linear
    const float driveGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("driveDb")->load());
    const float inGain  = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("inputGainDb")->load());
    const float outGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGainDb")->load());

    buffer.applyGain(inGain);
    // Atualizar o pico de entrada antes de aplicar a distorção
    peakInputLinear.store(buffer.getMagnitude(0, buffer.getNumSamples()));

    //Aplicar drive simples, transformar em método depois:
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto *samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            samples[sample] *= std::tanh(samples[sample]* driveGain);
        }
    }

    buffer.applyGain(outGain);
    // Atualizar o pico de saída após aplicar a distorção e o ganho de saída
    peakOutputLinear.store(buffer.getMagnitude(0, buffer.getNumSamples()));
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
