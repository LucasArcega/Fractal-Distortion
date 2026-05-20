#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utils/ConversionUtils.h"

juce::AudioProcessorValueTreeState::ParameterLayout FractalDistortionAudioProcessor::createParameterLayout()
{
 
    auto range = juce::NormalisableRange<float>(-60.f, 12.f, 0.1f, 0.45f);
    auto attrs = juce::AudioParameterFloatAttributes{}
                     .withStringFromValueFunction(fractal_utils::gainDbToText)
                     .withValueFromStringFunction(fractal_utils::textToGainDb);

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "inputGainDb", 1 }, juce::translate("Entrada"), range, 0.f, attrs));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "outputGainDb", 1 }, juce::translate("Saida"), range, 0.f, attrs));

    return layout;
}


/**
 * @brief Construtor do processador.
 *
 * @details Configura layout de buses (estéreo in/out) e inicializa o APVTS.
 * O layout de parâmetros é criado via createParameterLayout().
 *
 * @note isBusesLayoutSupported() limita o plugin a mono ou estéreo apenas.
 */
FractalDistortionAudioProcessor::FractalDistortionAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput(juce::translate("Entrada"), juce::AudioChannelSet::stereo(), true)
          .withOutput(juce::translate("Saida"), juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

/**
 * @brief Prepara o DSP para processamento (chamado ao iniciar playback ou mudar sample rate).
 *
 * @details Inicializa:
 * - Taxa de amostragem atual
 * - Tamanho da janela RMS (~43ms, padrão Airwindows)
 * - Reseta medidores de pico
 *
 * @param sampleRate      Taxa de amostragem do host (ex: 44100, 48000)
 * @param samplesPerBlock Tamanho máximo do bloco de áudio
 *
 * @note Esta é a ÚNICA função que aloca memória. processBlock() nunca aloca.
 */
void FractalDistortionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    rmsSize  = static_cast<int>((1882.0 / 44100.0) * currentSampleRate);
    rmsCount = 0;
    peakIn        = 0.f;
    peakOut       = 0.f;
    peakOutLeft  = 0.f;
    peakOutRight = 0.f;
}

/**
 * @brief Verifica se o layout de buses é suportado.
 *
 * @details Este plugin suporta APENAS:
 * - Mono → Mono
 * - Estéreo → Estéreo
 *
 * Layouts assimétricos (ex: mono→estéreo) são rejeitados.
 *
 * @param layouts Layout proposto pelo host
 * @return true se suportado, false caso contrário
 */
bool FractalDistortionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();
    if (out != in) return false;
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

/**
 * @brief Processa um bloco de áudio (real-time, thread crítica).
 *
 * @details Fluxo de processamento por amostra:
 * 1. Aplica ganho de entrada (inputGainDb)
 * 2. Mede picos de entrada
 * 3. Aplica ganho de Output (outputGainDb)
 * 4. Mede picos de Output (L, R, max)
 * 5. A cada ~43ms (rmsSize amostras), envia mensagens para UI
 *
 * @param buffer Buffer de áudio (modificado in-place)
 * @param midi   Buffer MIDI (ignorado, plugin é audio-only)
 *
 * @warning NUNCA aloque memória, trave locks ou chame funções bloqueantes aqui!
 */
void FractalDistortionAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    juce::ScopedNoDenormals noDenormals;

    const float inGain  = juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue("inputGainDb")->load());
    const float outGain = juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue("outputGainDb")->load());

    buffer.applyGain(inGain);

    const int nCh = buffer.getNumChannels();
    const int nSm = buffer.getNumSamples();

    for (int ch = 0; ch < nCh; ++ch)
    {
        const float* r = buffer.getReadPointer(ch);
        for (int i = 0; i < nSm; ++i)
            peakIn = juce::jmax(peakIn, std::abs(r[i]));
    }

    buffer.applyGain(outGain);

    // Picos de Output: medem o sinal com gain de Output aplicado (o que vai ao conversor D/A).
    if (nCh >= 1)
    {
        const float* p0 = buffer.getReadPointer(0);
        for (int i = 0; i < nSm; ++i)
            peakOutLeft = juce::jmax(peakOutLeft, std::abs(p0[i]));
    }
    if (nCh >= 2)
    {
        const float* p1 = buffer.getReadPointer(1);
        for (int i = 0; i < nSm; ++i)
            peakOutRight = juce::jmax(peakOutRight, std::abs(p1[i]));
    }
    else
    {
        // Mono: espelha L no R para os dois medidores do rodapé
        peakOutRight = peakOutLeft;
    }

    peakOut = juce::jmax(peakOutLeft, peakOutRight);

    rmsCount += buffer.getNumSamples();

    if (rmsCount >= rmsSize)
    {
        AudioToUIMessage msg;

        msg.what = AudioToUIMessage::PEAK_IN;
        msg.newValue = peakIn;
        audioToUI.push(msg);

        msg.what = AudioToUIMessage::PEAK_OUT;
        msg.newValue = peakOut;
        audioToUI.push(msg);

        msg.what = AudioToUIMessage::PEAK_OUT_LEFT;
        msg.newValue = peakOutLeft;
        audioToUI.push(msg);

        msg.what = AudioToUIMessage::PEAK_OUT_RIGHT;
        msg.newValue = peakOutRight;
        audioToUI.push(msg);

        msg.what = AudioToUIMessage::INCREMENT;
        msg.newValue = 0.f;
        audioToUI.push(msg);

        peakIn         = 0.f;
        peakOut        = 0.f;
        peakOutLeft    = 0.f;
        peakOutRight   = 0.f;
        rmsCount       = 0;
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
