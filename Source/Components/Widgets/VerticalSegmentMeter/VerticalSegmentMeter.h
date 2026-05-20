#pragma once

#include <JuceHeader.h>

namespace GUI
{

/**
 * @brief Medidor vertical segmentado (estilo LED) para nível de áudio.
 *
 * @details Visual:
 * - **22 segmentos LED** verticais (de baixo para cima)
 * - Segmentos acesos: roxo/lavanda (0xffb388ff, alpha 0.92)
 * - Segmentos apagados: cinza escuro (0xff2a2f3a)
 * - Label no topo: "IN" ou "OUT" (10pt, branco)
 * - Frame: fundo escuro + borda sutil + cantos redondos
 *
 * @section usage Uso
 *
 * Usado no OutputPanel::MeterCluster para medidores IN e OUT.
 * Atualizado em MeterCluster::updateLevels() com valores vindos do processBlock().
 *
 * @section led_mapping Mapeamento LED
 *
 * - Faixa: -60 dB (fundo de escala) … +6 dB (topo)
 * - 22 LEDs dividem os 66 dB linearmente (~3 dB por LED)
 * - Nível normalizado: (levelDb + 60) / 66 → [0, 1]
 * - LEDs acesos: floor(normalized × 22)
 *
 * @section optimization Otimização de Repaint
 *
 * setLevelDb() só chama repaint() se o valor mudar > 0.05 dB,
 * evitando repaints desnecessários e melhorando performance.
 *
 * @note Coordenadas: LED 0 (bottom) = -60 dB, LED 21 (top) = +6 dB
 */
class VerticalSegmentMeter : public juce::Component
{
public:
    /**
     * @brief Construtor.
     *
     * @param channelLabelText Texto curto no topo (ex: "IN", "OUT")
     */
    explicit VerticalSegmentMeter(juce::String channelLabelText);

    /**
     * @brief Desenha o medidor (label + frame + 22 segmentos LED).
     *
     * @details Ordem de rendering:
     * 1. Label do canal (topo, 14px)
     * 2. Frame (fundo escuro + borda)
     * 3. 22 segmentos LED (de baixo para cima)
     */
    void paint(juce::Graphics& g) override;

    /**
     * @brief Atualiza o nível do medidor.
     *
     * @details Só dispara repaint() se o valor mudar > 0.05 dB.
     *
     * @param levelDb Nível em dB (-60 … +6)
     */
    void setLevelDb(float levelDb) noexcept;

    /** @brief Retorna o nível atual em dB. */
    [[nodiscard]] float getLevelDb() const noexcept { return levelDb; }

private:
    juce::String channelLabelText;  ///< Label do canal ("IN" ou "OUT")
    float levelDb { -60.f };        ///< Nível atual em dB (default: silêncio)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerticalSegmentMeter)
};

} // namespace GUI
