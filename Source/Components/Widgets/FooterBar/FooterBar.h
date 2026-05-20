#pragma once

#include <JuceHeader.h>

#include "Components/Widgets/HorizontalMetter/HorizontalMetter.h"

namespace GUI
{
/**
 * @brief Rodapé com medidores estéreo da Output (L/R).
 *
 * @details Layout:
 * - Dois medidores horizontais empilhados verticalmente
 * - L (topo): canal esquerdo
 * - R (baixo): canal direito
 * - Gap de 4px entre os medidores
 * - Altura total: 52px (24px cada medidor + 4px gap)
 *
 * @section usage Uso
 *
 * Posicionado na parte inferior da janela (PluginEditor).
 * Atualizado em PluginEditor::idle() com valores de pico vindos do processBlock().
 *
 * @note Em configuração mono, ambos os medidores mostram o mesmo valor (L=R).
 */
class FooterBar : public juce::Component
{
public:
    /** @brief Construtor - inicializa os dois medidores horizontais. */
    FooterBar();

    /** @brief Desenha o fundo do rodapé. */
    void paint(juce::Graphics& g) override;

    /** @brief Posiciona os medidores L/R (layout vertical). */
    void resized() override;

    /** @brief Retorna referência ao medidor do canal esquerdo. */
    HorizontalMetter& getOutputMeterLeft() noexcept { return outputMeterLeft; }

    /** @brief Retorna referência ao medidor do canal direito. */
    HorizontalMetter& getOutputMeterRight() noexcept { return outputMeterRight; }

private:
    HorizontalMetter outputMeterLeft;   ///< Medidor do canal esquerdo (topo)
    HorizontalMetter outputMeterRight;  ///< Medidor do canal direito (baixo)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterBar)
};
} // namespace GUI
