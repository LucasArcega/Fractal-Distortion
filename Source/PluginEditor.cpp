#include "PluginEditor.h"
#include "Utils/ConversionUtils.h"

namespace
{
    /** @brief Altura da faixa inferior com os painéis (Output). */
    constexpr int kBottomStripHeight = 236;

    /** @brief Altura do rodapé com medidores estéreo L/R. */
    constexpr int kFooterHeight = 52;

    /** @brief Espaçamento vertical entre a faixa inferior e o rodapé. */
    constexpr int kFooterGap = 8;

    /**
     * @brief Layout da coluna central: estica o painel para preencher toda a área.
     */
    void layoutCenterColumn(juce::Component& column, juce::Component& panel)
    {
        juce::FlexBox fb;
        fb.flexDirection  = juce::FlexBox::Direction::row;
        fb.justifyContent = juce::FlexBox::JustifyContent::center;
        fb.alignItems     = juce::FlexBox::AlignItems::stretch;
        fb.items.add(juce::FlexItem(panel).withFlex(1.f));
        fb.performLayout(column.getLocalBounds().toFloat());
    }
} // namespace

/**
 * @brief Construtor do editor.
 *
 * @details Inicializa todos os componentes, aplica estilos e cria attachments APVTS.
 * Ordem de inicialização:
 * 1. Define tamanho da janela (960×684)
 * 2. Cria e adiciona componentes de chrome (header, footer)
 * 3. Cria colunas da grid central (IN | CENTER | OUT)
 * 4. Cria painéis inferiores (Delay Core, Character, Output)
 * 6. **ÚLTIMO**: Inicia IdleTimer (~30Hz) para drenar fila audio→UI
 *
 * @param p Referência ao processador (deve sobreviver ao editor)
 *
 * @warning IdleTimer só é iniciado APÓS todos os componentes serem adicionados!
 */
FractalDistortionAudioProcessorEditor::FractalDistortionAudioProcessorEditor(FractalDistortionAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
    , distortionPanel(p.getAPVTS())
    , inputGainPanel(p.getAPVTS(), "IN", "IN: ---", ParameterIDs::inputGainDb.getParamID())
    , outputGainPanel(p.getAPVTS(), "OUT", "OUT: ---", ParameterIDs::outputGainDb.getParamID())
{
    setSize(960, 684);

    addAndMakeVisible(footerBar);

    addAndMakeVisible(inColumn);
    addAndMakeVisible(centerColumn);
    addAndMakeVisible(outColumn);

    inColumn.addAndMakeVisible(inputGainPanel);
    centerColumn.addAndMakeVisible(distortionPanel);
    outColumn.addAndMakeVisible(outputGainPanel);
    
    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);
}

FractalDistortionAudioProcessorEditor::~FractalDistortionAudioProcessorEditor()
{
    idleTimer->stopTimer();
}

void FractalDistortionAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0e1015));

    const int footerY = footerBar.getY();
    g.setColour(juce::Colour(0xffb388ff).withAlpha(0.35f));
    g.drawHorizontalLine(footerY, 0.f, (float) getWidth());
}

/**
 * @brief Calcula e aplica layout de todos os componentes.
 *
 * @details Ordem de layout (de baixo para cima):
 * 1. **Rodapé** (FooterBar): 52px na parte inferior
 * 2. **Gap**: 8px entre rodapé e painéis inferiores
 * 3. **Faixa inferior** (Bottom Strip): 236px com 3 painéis lado a lado
 *    - DelayCorePanel: 40% da largura
 *    - CharacterPanel + OutputPanel: dividem os 60% restantes
 * 4. **Header** (HeaderStrip): 44px no topo
 * 5. **Grid central** (3 colunas): espaço restante
 *    - IN (Fr 100): slider de ganho vertical
 *    - CENTER (Fr 145): TapEditor (maior, área de trabalho principal)
 *    - OUT (Fr 100): label de pico
 *
 * @note Fr (fractions) em JUCE 8 são INTEIROS, não floats.
 *       Fr(100) + Fr(145) + Fr(100) = proporção 100:145:100
 */
void FractalDistortionAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    auto footerArea = area.removeFromBottom(kFooterHeight);
    footerBar.setBounds(footerArea);

    area.removeFromBottom(kFooterGap);

    const auto bounds = area.reduced(12, 10);

    juce::Grid grid;
    using Track = juce::Grid::TrackInfo;
    using Fr    = juce::Grid::Fr;
    using Px    = juce::Grid::Px;

    grid.templateRows    = { Track(Fr(1)) };
    grid.templateColumns = { Track(Fr(100)), Track(Fr(145)), Track(Fr(100)) };
    grid.columnGap = Px(10);

    grid.items = {
        juce::GridItem(inColumn).withArea(1, 1),
        juce::GridItem(centerColumn).withArea(1, 2),
        juce::GridItem(outColumn).withArea(1, 3),
    };

    grid.performLayout(bounds);

    inputGainPanel.setBounds(inColumn.getLocalBounds());
    layoutCenterColumn(centerColumn, distortionPanel);
    outputGainPanel.setBounds(outColumn.getLocalBounds());
}

void FractalDistortionAudioProcessorEditor::idle()
{
    inputGainPanel.updatePeakLabel(audioProcessor.getPeakInputLinear());
    outputGainPanel.updatePeakLabel(audioProcessor.getPeakOutputLinear());
}

