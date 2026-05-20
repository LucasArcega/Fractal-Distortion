#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

#include "PluginProcessor.h"

// --- Smoke: instância e parâmetros registrados ---
TEST_CASE("FractalDistortionAudioProcessor instantiates", "[processor]")
{
    FractalDistortionAudioProcessor processor;

    REQUIRE_FALSE(processor.getName().isEmpty());
    REQUIRE(processor.getAPVTS().getParameter("inputGainDb") != nullptr);
    REQUIRE(processor.getAPVTS().getParameter("outputGainDb") != nullptr);
}

// --- Layout de canais suportado pelo plugin ---
TEST_CASE("Stereo bus layout is supported", "[processor]")
{
    FractalDistortionAudioProcessor processor;

    juce::AudioProcessor::BusesLayout stereo;
    stereo.inputBuses.add(juce::AudioChannelSet::stereo());
    stereo.outputBuses.add(juce::AudioChannelSet::stereo());

    REQUIRE(processor.isBusesLayoutSupported(stereo));
}

TEST_CASE("Layout de quatro canais arbitrario e rejeitado", "[processor]")
{
    FractalDistortionAudioProcessor processor;

    juce::AudioProcessor::BusesLayout quad;
    quad.inputBuses.add(juce::AudioChannelSet::quadraphonic());
    quad.outputBuses.add(juce::AudioChannelSet::quadraphonic());

    REQUIRE_FALSE(processor.isBusesLayoutSupported(quad));
}

// --- Ciclo prepare/release não deve lançar exceção ---
TEST_CASE("prepareToPlay and releaseResources", "[processor]")
{
    FractalDistortionAudioProcessor processor;

    REQUIRE_NOTHROW(processor.prepareToPlay(44100.0, 512));
    REQUIRE_NOTHROW(processor.prepareToPlay(48000.0, 64));
    REQUIRE_NOTHROW(processor.releaseResources());
}

// --- Editor: precisa de vídeo ---
TEST_CASE("Editor can be created and destroyed", "[processor][gui]")
{
    juce::ScopedJuceInitialiser_GUI juceGui;

    if (std::getenv("SKIP_FRACTAL_DELAY_GUI_TESTS") != nullptr)
        SKIP("SKIP_FRACTAL_DELAY_GUI_TESTS definido");

    if (juce::Desktop::getInstance().isHeadless())
        SKIP("Sem display; teste do editor de GUI ignorado");

    FractalDistortionAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    REQUIRE(editor != nullptr);
    REQUIRE(editor->getWidth() > 0);
    REQUIRE(editor->getHeight() > 0);

#if JUCE_MODAL_LOOPS_PERMITTED
    if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
    {
        const auto deadline = juce::Time::getMillisecondCounter() + 2000u;
        while (juce::Time::getMillisecondCounter() < deadline)
        {
            if (! mm->runDispatchLoopUntil(50))
                break;
        }
    }
#endif

    editor.reset();
    processor.releaseResources();
}