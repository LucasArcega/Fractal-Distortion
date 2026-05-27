#pragma once
#include <JuceHeader.h>

namespace DSP {

    class TubeDistortion {
    public:
        void prepare(double newSampleRate) noexcept {
            sampleRate = newSampleRate;
            updateFilters();
        }

        void reset() noexcept {
            this->hpState = 0.f;
            this->hpLastInput = 0.f;
            this->lpState = 0.f;
        }

        /// <summary>
        /// Define o ganho de drive em decibéis. O ganho de drive controla a quantidade de distorção
        /// aplicada ao sinal. NÃO LINEARIZAR ANTES!
        /// </summary>
        /// <param name="driveDb">Ganho de drive em decibéis.</param>
        void setDrive(float driveDb) noexcept {
            this->driveGain = juce::Decibels::decibelsToGain(driveDb);
        }

        void setBias(float newBias) noexcept { this->bias = juce::jlimit(0.f, 0.5f, newBias); }

        void setToneHz(float hz) noexcept {
            this->toneHz = juce::jlimit(1000.f, 20000.f, hz);
            this->updateFilters();
        }

        float processSample(float input) noexcept {

            // Pipeline Tube clássico:
            // 1 - Limpa o sinal antes de dar ganho, removendo rumble e DC indesejado.
            const float filteredInput = highPass(input);

            // 2 - Aplica ganho e distorção não linear, simulando a saturação de uma válvula.
            const float biasedInput = filteredInput + bias; // Adiciona assimetria

            // 3. Amplificação: Empurra o sinal contra o "teto" do circuito
            const float drivenInput = biasedInput * driveGain;

            // 4. Ondulação (Waveshaping): Modifica a forma da onda usando a tangente hiperbólica
            // Satura, mas compensa o ganho na saída para não virar fuzz
            const float saturated = std::tanh(drivenInput) / std::max(driveGain * 0.5f, 1.f);
            

            // 5. Restauração: Remove o deslocamento DC que a própria distorção gerou.
            // O bias adiciona um deslocamento DC ao sinal, e a saturação pode amplificar esse
            // deslocamento, resultando em um aumento indesejado do nível DC na saída. Para corrigir
            // isso, calculamos o deslocamento DC gerado pela saturação usando a função tanh
            // aplicada ao bias multiplicado pelo ganho de drive. Em seguida, subtraímos esse
            // deslocamento do sinal saturado para restaurar o equilíbrio DC original.
            const float dcOffset = std::tanh(bias * driveGain) / std::max(driveGain * 0.5f, 1.f);
            
            const float corrected = saturated - dcOffset; // Remove DC gerado

            // 6. Polimento: Suaviza os harmônicos agudos gerados pela saturação
            const float toned = lowPass(corrected); // Suaviza agudos

            return toned;
        }

    private:
        void updateFilters() noexcept {
            const float highPassHz = 30.f;

            // Cálculo do coeficiente de filtro highPass usando a fórmula de um filtro RC simples
            hpAlpha = std::exp(-juce::MathConstants<float>::twoPi * highPassHz /
                               static_cast<float>(sampleRate));

            // Cálculo do coeficiente de filtro lowPass usando a fórmula de um filtro RC simples
            lpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * toneHz /
                                      static_cast<float>(sampleRate));
        }

        float highPass(float input) noexcept {

            const float signalDifference = this->computeSignalDifference(input, hpLastInput);

            // O sinal com memória é a combinação da diferença atual (que captura as mudanças
            // rápidas) e o estado anterior (que mantém a resposta suave). Isso permite que o filtro
            // reaja rapidamente a mudanças no sinal, mas também mantenha uma resposta suave e
            // natural, evitando artefatos indesejados.
            const float signalWithMemory = this->hpState + signalDifference;

            // O resultado final é o sinal filtrado, onde o hpAlpha controla a quantidade de
            // filtragem aplicada. O estado do filtro é atualizado para a próxima iteração,
            // garantindo que o filtro mantenha sua resposta ao longo do tempo. Seu decaimento é
            // suave e exponencial assim como um filtro RC clássico, o que ajuda a evitar artefatos
            // de filtragem agressiva e mantém a musicalidade do som.
            const float output = this->hpAlpha * signalWithMemory;

            this->hpState = output;
            this->hpLastInput = input;
            return output;
        }

        /// <summary>
        /// Applies a low-pass filter to the input signal and updates the internal filter state.
        /// </summary>
        /// <param name="input">The new input value to be filtered.</param>
        /// <returns>The updated low-pass filtered value.</returns>
        float lowPass(float input) noexcept {

            // apenas altera lpState caso haja diferença entre input e lpState.
            // se input for igual a lpState, o resultado é 0 e lpState permanece inalterado,
            // evitando cálculos desnecessários.

            float signalDifference = this->computeSignalDifference(input, this->lpState);

            this->lpState += this->lpCoeff * signalDifference;

            return this->lpState;
        }

        /// <summary>
        /// Calculates the difference between the current and previous signal values.
        /// </summary>
        /// <param name="input">The current signal value.</param>
        /// <param name="lastInput">The previous signal value.</param>
        /// <returns>The difference between the current and previous signal values.</returns>
        float computeSignalDifference(float input, float lastInput) noexcept {
            return input - lastInput;
        }

        double sampleRate = 44100.0;

        float driveGain = 1.f;

        // Bias para simular a assimetria da válvula, controlando o ponto de operação
        float bias = 0.02f;
        float toneHz = 16000.f;

        float hpAlpha = 0.f;
        float hpState = 0.f;
        float hpLastInput = 0.f;

        float lpCoeff = 1.0f;
        float lpState = 0.f;
    };

} // namespace DSP