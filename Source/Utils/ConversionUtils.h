#pragma once

#include <JuceHeader.h>

/**
 * @brief Utilitários de conversão e formatação para o plugin Fractal-Distortion.
 *
 * @details Funções reutilizáveis para:
 * - Conversão linear ↔ dB
 * - Formatação de valores de pico
 * - Conversão de percentuais
 *
 * Centraliza lógica duplicada encontrada em PluginEditor e OutputPanel.
 */
namespace fractal_utils {

    /**
     * @brief Converte ganho linear para dB na faixa dos medidores (-60…+6 dB).
     *
     * @details Valores muito baixos (< 1e-8 ≈ -160 dB) são clamped para -60 dB
     * para evitar valores extremos na UI visual.
     *
     * @param linearGain Amplitude linear (0…∞)
     * @return Nível em dB, clamped para [-60, +6]
     *
     * @note Esta função é usada por medidores visuais (HorizontalMetter, VerticalSegmentMeter)
     *       e labels de pico. O threshold de 1e-8 garante que o silêncio digital seja
     *       tratado como -60 dB em vez de -inf (mais legível visualmente).
     */
    inline float linearToMeterDb(float linearGain) noexcept {
        if (linearGain < 1e-8f)
            return -60.f;
        return juce::jlimit(-60.f, 6.f, juce::Decibels::gainToDecibels(linearGain));
    }

    /**
     * @brief Formata valor de pico linear como string legível.
     *
     * @details Formatos de Output:
     * - Silêncio (< 1e-8): "-inf"
     * - Níveis audíveis: "X.X" (1 casa decimal, sem sufixo "dB")
     *
     * Usado em labels de pico que exibem o valor numérico abaixo dos medidores.
     *
     * @param linearGain Amplitude linear (0…∞)
     * @return String formatada (ex: "-12.3", "-inf")
     *
     * @note Não inclui o sufixo "dB" para economizar espaço visual (labels pequenos).
     */
    inline juce::String formatPeakReadout(float linearGain) {
        if (linearGain < 1e-8f)
            return "-inf";
        return juce::String(juce::Decibels::gainToDecibels(linearGain), 1);
    }

    inline juce::String frequencyToText(float hz, int) {
        if (hz >= 1000.0f)
            return juce::String(hz / 1000.0f, 1) + " kHz";
        return juce::String(juce::roundToInt(hz)) + " Hz";
    }

    /**
     * @brief Formata valor de pico linear como string com sufixo "dB".
     *
     * @details Similar a formatPeakReadout(), mas inclui o sufixo " dB".
     * Usado em labels maiores (ex: "IN: -12.3 dB") onde há espaço suficiente.
     *
     * @param linearGain Amplitude linear (0…∞)
     * @return String formatada (ex: "-12.3 dB", "-inf dB")
     */
    inline juce::String formatPeakWithDb(float linearGain) {
        if (linearGain < 1e-8f)
            return "-inf dB";
        return juce::String(juce::Decibels::gainToDecibels(linearGain), 1) + " dB";
    }

    /**
     * @brief Converte valor normalizado (0…1) para string percentual.
     *
     * @details Usado em sliders de mix, feedback, etc. que exibem valores como "50 %".
     *
     * @param normalized Valor normalizado (0…1)
     * @return String formatada (ex: "50 %", "100 %")
     */
    inline juce::String normalizedToPercentString(float normalized) {
        return juce::String(juce::roundToInt(juce::jlimit(0.f, 1.f, normalized) * 100.f)) + " %";
    }

    /**
     * @brief Converte string percentual de volta para valor normalizado.
     *
     * @details Aceita formatos variados:
     * - "50", "50 ", "50%", "50 %"
     * - Case-insensitive
     *
     * @param text String digitada pelo usuário
     * @return Valor normalizado (0…1), clamped
     */
    inline float percentStringToNormalized(const juce::String &text) {
        auto t = text.trim();
        if (t.endsWithIgnoreCase("%"))
            t = t.dropLastCharacters(1).trim();
        return juce::jlimit(0.f, 1.f, t.getFloatValue() * 0.01f);
    }

    /** @brief Threshold de silêncio em dB para conversão de parâmetros de ganho. */
    constexpr float silenceDb = -80.f;

    /**
     * @brief Converte valor em dB para texto exibido no controle e no host (DAW).
     *
     * @details Valores <= -79 dB são exibidos como "-inf dB".
     *
     * @param value Valor em dB
     * @return String formatada (ex: "0.0 dB", "-inf dB")
     */
    inline juce::String gainDbToText(float value, int) {
        if (value <= silenceDb + 1.f)
            return "-inf dB";
        return juce::String(value, 1) + " dB";
    }

    /**
     * @brief Converte texto digitado pelo usuário para valor em dB.
     *
     * @details Aceita formatos variados:
     * - "0", "-6.0", "+3.5" (com ou sem sufixo "dB")
     * - "-inf", "-infinity" (silêncio)
     * - Texto inválido ("abc", "") → fallback para silenceDb
     *
     * @param text String digitada pelo usuário
     * @return Valor em dB clamped para a faixa [-60, +12] dB
     */
    inline float textToGainDb(const juce::String &text) {
        auto t = text.trim().toLowerCase();

        if (t.endsWith("db"))
            t = t.dropLastCharacters(2).trim();

        if (t == "-inf" || t == "-infinity" || t == "inf" || t.isEmpty())
            return silenceDb;

        const float parsed = t.getFloatValue();

        const bool looksNumeric =
            t[0] == '-' || t[0] == '+' || juce::CharacterFunctions::isDigit(t[0]);
        if (!looksNumeric)
            return silenceDb;

        return juce::jlimit(-60.f, 12.f, parsed);
    }

     /**
     * @brief Formata valor normalizado (0…1) como percentual legível.
     *
     * @details Exemplos:
     * - 0.0  → "0%"
     * - 0.5  → "50%"
     * - 1.0  → "100%"
     *
     * @param value Valor normalizado (0…1)
     * @param Segundo parâmetro ignorado (assinatura exigida pelo APVTS)
     * @return String formatada (ex: "75%")
     */
    inline juce::String normalizedToPercent(float value, int) {
        return juce::String(juce::roundToInt(value * 100.f)) + "%";
    }

} // namespace fractal_utils
