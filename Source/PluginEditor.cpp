#include "PluginEditor.h"
#include "Utils/ConversionUtils.h"

namespace
{
    /**
     * @brief Dimensões fixas do layout baseadas no mockup Figma.
     *
     * Janela padrão: 960×684 pixels (~1.40 aspect ratio)
     */

    /** @brief Altura da barra superior (marca, tabs, preset, BPM/TAP/SYNC). */
    constexpr int kHeaderHeight = 44;

    /** @brief Altura da faixa inferior com os 3 painéis (Delay Core | Character | Output). */
    constexpr int kBottomStripHeight = 236;

    /** @brief Altura do rodapé com medidores estéreo L/R. */
    constexpr int kFooterHeight = 52;

    /** @brief Espaçamento vertical entre a faixa inferior e o rodapé. */
    constexpr int kFooterGap = 8;

    /**
     * @brief Aplica estilo visual ao slider vertical de ganho (IN column).
     *
     * @details Estilo monocromático com fundo escuro, track laranja, thumb branco.
     * TextBox abaixo do slider para exibir valor em dB.
     */
    void applyMonoSliderStyle(juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 22);
        s.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff22262e));
        s.setColour(juce::Slider::trackColourId, juce::Colour(0xffff6b2d));
        s.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe8eaed));
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff12151c));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }

    /**
     * @brief Aplica estilo ao título pequeno (ex: "IN", "OUT").
     *
     * @param l    Label a estilizar
     * @param text Texto a exibir
     */
    void styleSmallTitle(juce::Label& l, const juce::String& text)
    {
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(juce::FontOptions(14.f)).boldened());
        l.setColour(juce::Label::textColourId, juce::Colour(0xffe8eaed));
    }

    /**
     * @brief Aplica estilo ao label de pico (ex: "IN: -12.3 dB").
     *
     * @param l       Label a estilizar
     * @param initial Texto inicial (será atualizado em idle())
     */
    void stylePeakLabel(juce::Label& l, const juce::String& initial)
    {
        l.setText(initial, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(juce::FontOptions(12.f)));
        l.setColour(juce::Label::textColourId, juce::Colour(0xff9aa7b8));
    }

    /**
     * @brief Layout vertical da coluna IN: título + label de pico + slider.
     *
     * @details Usa FlexBox com direção column, slider flexível ocupa espaço restante.
     */
    void layoutSideStrip(juce::Component& column,
                         juce::Label& title,
                         juce::Label& peak,
                         juce::Slider& slider)
    {
        auto r = column.getLocalBounds().toFloat();
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::column;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        fb.alignItems     = juce::FlexBox::AlignItems::stretch;

        constexpr float titleH = 20.f;
        constexpr float peakH  = 22.f;
        constexpr float sliderMinH = 120.f;

        fb.items.add(juce::FlexItem(title).withHeight(titleH));
        fb.items.add(juce::FlexItem(peak).withHeight(peakH));
        fb.items.add(juce::FlexItem(slider).withFlex(1.f).withMinHeight(sliderMinH).withMaxWidth((float) column.getWidth()));

        fb.performLayout(r);
    }

    /**
     * @brief Layout da coluna OUT: título + label de pico (sem slider).
     *
     * @details O slider de ganho de Output está no OutputPanel, não na coluna OUT.
     *
     * @param column Componente pai (outColumn)
     * @param title  Label "OUT"
     * @param peak   Label "OUT: X dB"
     */
    void layoutOutPeakColumn(juce::Component& column, juce::Label& title, juce::Label& peak)
    {
        auto r = column.getLocalBounds().toFloat();
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::column;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        fb.alignItems     = juce::FlexBox::AlignItems::stretch;
        constexpr float titleH = 20.f;
        constexpr float peakH  = 22.f;
        fb.items.add(juce::FlexItem(title).withHeight(titleH));
        fb.items.add(juce::FlexItem(peak).withHeight(peakH));
        fb.items.add(juce::FlexItem().withFlex(1.f));
        fb.performLayout(r);
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
 * 5. Cria attachments APVTS para sliders
 * 6. **ÚLTIMO**: Inicia IdleTimer (~30Hz) para drenar fila audio→UI
 *
 * @param p Referência ao processador (deve sobreviver ao editor)
 *
 * @warning IdleTimer só é iniciado APÓS todos os componentes serem adicionados!
 */
FractalDistortionAudioProcessorEditor::FractalDistortionAudioProcessorEditor(FractalDistortionAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
    , outputPanel(p)
{
    // Dimensão padrão da janela (baseada no mockup Figma)
    setSize(960, 684);
    addAndMakeVisible(footerBar);

    addAndMakeVisible(inColumn);
    addAndMakeVisible(centerColumn);
    addAndMakeVisible(outColumn);

    styleSmallTitle(inTitle, "IN");
    stylePeakLabel(inLabel, "IN: ---");
    applyMonoSliderStyle(inputSlider);
    inColumn.addAndMakeVisible(inTitle);
    inColumn.addAndMakeVisible(inLabel);
    inColumn.addAndMakeVisible(inputSlider);

    styleSmallTitle(outTitle, "OUT");
    stylePeakLabel(outLabel, "OUT: ---");
    outColumn.addAndMakeVisible(outTitle);
    outColumn.addAndMakeVisible(outLabel);

    addAndMakeVisible(outputPanel);

    inputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "inputGainDb", inputSlider);

    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);
}

FractalDistortionAudioProcessorEditor::~FractalDistortionAudioProcessorEditor()
{
    idleTimer->stopTimer();
}

void FractalDistortionAudioProcessorEditor::idle()
{
    FractalDistortionAudioProcessor::AudioToUIMessage msg;
    bool needsRepaint = false;

    while (audioProcessor.audioToUI.pop(msg))
    {
        switch (msg.what)
        {
            case FractalDistortionAudioProcessor::AudioToUIMessage::PEAK_IN:
                currentPeakIn = msg.newValue;
                break;
            case FractalDistortionAudioProcessor::AudioToUIMessage::PEAK_OUT:
                currentPeakOut = msg.newValue;
                break;
            case FractalDistortionAudioProcessor::AudioToUIMessage::PEAK_OUT_LEFT:
                currentPeakOutLeft = msg.newValue;
                break;
            case FractalDistortionAudioProcessor::AudioToUIMessage::PEAK_OUT_RIGHT:
                currentPeakOutRight = msg.newValue;
                break;
            case FractalDistortionAudioProcessor::AudioToUIMessage::INCREMENT:
                needsRepaint = true;
                break;
        }
    }

    if (needsRepaint)
    {
        // Usa ConversionUtils para formatação padronizada
        inLabel.setText("IN: " + fractal_utils::formatPeakWithDb(currentPeakIn), juce::dontSendNotification);
        outLabel.setText("OUT: " + fractal_utils::formatPeakWithDb(currentPeakOut), juce::dontSendNotification);

        // Atualiza medidores do rodapé (conversão linear → dB)
        footerBar.getOutputMeterLeft().setLevel(fractal_utils::linearToMeterDb(currentPeakOutLeft));
        footerBar.getOutputMeterRight().setLevel(fractal_utils::linearToMeterDb(currentPeakOutRight));

        // Atualiza medidores do OutputPanel
        outputPanel.updateOutputMetering(currentPeakIn, currentPeakOut);
    }
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

    // Layout de baixo para cima (remove* modifica 'area')
    auto footerArea = area.removeFromBottom(kFooterHeight);
    footerBar.setBounds(footerArea);

    area.removeFromBottom(kFooterGap);

    // Bottom strip: 3 painéis (Delay Core | Character | Output)
    auto bottomStrip = area.removeFromBottom(kBottomStripHeight);
    auto b = bottomStrip.reduced(6, 6);
    const int innerW = b.getWidth();
    // Delay Core compacto: 20% (MODE + PING PONG + 2 knobs lado a lado)
    const int delayCorePanelWidth = juce::jmax(180, juce::roundToInt(innerW * 0.20f));
    const int pairW = innerW - delayCorePanelWidth;
    const int charW = pairW / 2;

    outputPanel.setBounds(b);


    const auto bounds = area.reduced(12, 10);

    juce::Grid grid;
    using Track = juce::Grid::TrackInfo;
    using Fr    = juce::Grid::Fr;
    using Px    = juce::Grid::Px;

    grid.templateRows = { Track(Fr(1)) };
    grid.templateColumns = { Track(Fr(100)), Track(Fr(145)), Track(Fr(100)) };
    grid.columnGap = Px(10);

    grid.items = {
        juce::GridItem(inColumn).withArea(1, 1),
        juce::GridItem(centerColumn).withArea(1, 2),
        juce::GridItem(outColumn).withArea(1, 3),
    };

    grid.performLayout(bounds);

    layoutSideStrip(inColumn, inTitle, inLabel, inputSlider);
    layoutOutPeakColumn(outColumn, outTitle, outLabel);
}
