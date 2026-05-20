#pragma once
#include <JuceHeader.h>

/**
 * @brief Motor DSP principal do plugin Fractal Distortion.
 *
 * @details Esta classe é responsável por:
 * - Medir picos de entrada/Output e enviar dados para a UI via fila lock-free
 * - Gerenciar todos os parâmetros através do AudioProcessorValueTreeState (APVTS)
 *
 * @section audio_ui_communication Comunicação Audio → UI (Padrão Airwindows)
 *
 * Para evitar RAM explosion no Windows, usa-se uma fila lock-free que permite
 * comunicação assíncrona entre a thread de áudio e a UI:
 *
 * 1. **Thread de Áudio** (processBlock):
 *    - Calcula picos de entrada e Output
 *    - Envia mensagens para a fila (AudioToUIMessage)
 *    - Nunca aloca memória ou trava locks
 *
 * 2. **Thread de UI** (idle callback via IdleTimer ~30Hz):
 *    - Drena mensagens da fila
 *    - Atualiza medidores visuais
 *    - Chama repaint() APENAS quando valores mudam
 *
 * Padrão inspirado em: https://github.com/airwindows/Meter
 *
 */
class FractalDistortionAudioProcessor : public juce::AudioProcessor
{
public:
    FractalDistortionAudioProcessor();
    ~FractalDistortionAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    /**
     * @brief Retorna referência ao AudioProcessorValueTreeState (APVTS).
     *
     * O APVTS gerencia todos os parâmetros do plugin e permite attachments
     * automáticos com componentes UI (sliders, combos, etc.).
     */
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const noexcept { return apvts; }

    /**
     * @brief Mensagens enviadas da thread de áudio para a UI.
     *
     * @details Usa-se uma struct simples (POD) para comunicação lock-free.
     * A thread de áudio empurra (push) mensagens e a UI drena (pop) no idle().
     *
     * @note Este padrão evita alocações de memória e locks na thread de áudio,
     *       requisito crítico para processamento real-time sem glitches.
     */
    struct AudioToUIMessage
    {
        /** @brief Tipo da mensagem enviada para a UI. */
        enum What
        {
            PEAK_IN,        ///< Pico linear de entrada (após inputGain)
            PEAK_OUT,       ///< Pico máximo na Output (max de todos os canais) — medidor IN/OUT do OutputPanel
            PEAK_OUT_LEFT,  ///< Pico do canal esquerdo (medidor L do rodapé)
            PEAK_OUT_RIGHT, ///< Pico do canal direito (medidor R do rodapé)
            INCREMENT       ///< Marcador de fim de bloco RMS — sinal para UI atualizar visualmente
        } what { INCREMENT };
        float newValue = 0.f;  ///< Valor linear associado (amplitude de pico)
    };

    /**
     * @brief Fila lock-free de tamanho fixo para comunicação audio → UI.
     *
     * @tparam T     Tipo de mensagem (AudioToUIMessage neste caso)
     * @tparam qSize Capacidade máxima da fila (512 por padrão)
     *
     * @details Usa juce::AbstractFifo internamente para sincronização wait-free.
     * - push() é chamado pela thread de áudio (processBlock)
     * - pop() é chamado pela thread de UI (idle callback)
     *
     * @warning Se a fila encher (UI não drena rápido o suficiente), push() falha
     *          silenciosamente. Isto é preferível a bloquear a thread de áudio.
     */
    template <typename T, int qSize = 512>
    class LockFreeQueue
    {
    public:
        LockFreeQueue() : fifo(qSize) {}

        bool push(const T& item)
        {
            int start1, size1, start2, size2;
            fifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 < 1) return false;
            data[start1] = item;
            fifo.finishedWrite(1);
            return true;
        }

        bool pop(T& item)
        {
            int start1, size1, start2, size2;
            fifo.prepareToRead(1, start1, size1, start2, size2);
            if (size1 < 1) return false;
            item = data[start1];
            fifo.finishedRead(1);
            return true;
        }

    private:
        juce::AbstractFifo fifo;
        T data[qSize];
    };

    LockFreeQueue<AudioToUIMessage> audioToUI;

private:
    /** @brief Cria o layout de parâmetros do APVTS (chamado no construtor). */
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** @brief AudioProcessorValueTreeState - gerencia todos os parâmetros do plugin. */
    juce::AudioProcessorValueTreeState apvts;

    /** @brief Taxa de amostragem atual (atualizada em prepareToPlay). */
    double currentSampleRate = 44100.0;

    /**
     * @brief Contador de amostras processadas no bloco RMS atual.
     *
     * @details O metering usa janelas RMS de ~43ms (ver rmsSize).
     * Quando rmsCount >= rmsSize, envia mensagens para UI e reseta.
     */
    int rmsCount = 0;

    /**
     * @brief Tamanho da janela RMS em amostras (~43ms).
     *
     * @details Calculado em prepareToPlay() como:
     * rmsSize = (1882.0 / 44100.0) * sampleRate
     *
     * Padrão herdado do Airwindows Meter: janelas de ~43ms dão
     * atualização visual suave (~23Hz) sem sobrecarregar a fila.
     */
    int rmsSize = 1882;

    /** @brief Pico de entrada acumulado na janela RMS atual (linear, 0…∞). */
    float peakIn = 0.f;

    /** @brief Pico de Output geral acumulado (max de todos os canais). */
    float peakOut = 0.f;

    /** @brief Pico do canal esquerdo acumulado. */
    float peakOutLeft = 0.f;

    /** @brief Pico do canal direito acumulado. */
    float peakOutRight = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FractalDistortionAudioProcessor)
};
