#pragma once

#include <JuceHeader.h>

namespace GUI
{
    /**
     * @brief Medidor horizontal de nível de áudio (estilo barra de progresso).
     *
     * @details Visual:
     * - Fundo escuro (InputBackground)
     * - Barra de nível com gradiente lavanda/roxo (0xff6a4dcc → 0xffe8b4ff)
     * - Borda sutil (Border)
     * - Cantos redondos (3px)
     *
     * @section usage Uso
     *
     * Usado no FooterBar para medidores estéreo L/R do rodapé.
     * Atualizado em PluginEditor::idle() com valores vindos da thread de áudio.
     *
     * @section optimization Otimização de Repaint
     *
     * setLevel() só chama repaint() se o valor mudar mais que 0.02 dB,
     * evitando repaints desnecessários e melhorando performance.
     *
     * @note Faixa: -60 dB (silêncio) … +6 dB (clipping leve)
     *       Mapeamento linear visual (não logarítmico).
     */
    class HorizontalMetter : public juce::Component
    {
        public:
            /** @brief Desenha o medidor (fundo + borda + barra de nível). */
            void paint(juce::Graphics& g) override;

            /**
             * @brief Atualiza o nível do medidor.
             *
             * @details Só dispara repaint() se o valor mudar > 0.02 dB.
             *
             * @param levelDb Nível em dB (-60 … +6)
             */
            void setLevel(float levelDb) noexcept;

            /** @brief Retorna o nível atual em dB. */
            [[nodiscard]] float getLevel() const noexcept { return level; }

        private:
            float level = -60.f;  ///< Nível atual em dB (default: silêncio)
        };
} // namespace GUI
