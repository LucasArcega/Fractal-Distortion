#include "Components/Panels/OutputPanel/OutputPanel.h"

#include "Components/Widgets/StyleUtils.h"
#include "PluginProcessor.h"
#include "Utils/ConversionUtils.h"

namespace GUI
{
namespace
{
    constexpr juce::uint32 kPanelBorderColour = 0xff3d4450;
    constexpr juce::uint32 kAccentColour = 0xffb388ff;
    /** Largura mínima útil da caixa de texto sob o rotary (valor / %). */
    constexpr int kRotaryTextBoxWidthPx = 72;
    /** Início vertical da linha divisória no `paint()` — fica por baixo do título "OUTPUT". */
    constexpr float kDividerLineTopInsetPx = 22.f;
    /** Margem inferior da linha divisória relativamente ao fundo do painel desenhado. */
    constexpr float kDividerLineBottomInsetPx = 6.f;

    /**
     * @brief Anexa conversão percentual ao slider de mix (0…1 → "0 %" … "100 %").
     *
     * @details Agora usa funções de ConversionUtils em vez de lambdas inline.
     */
    void attachWetMixPercentDisplay(juce::Slider& wetMixSlider)
    {
        wetMixSlider.textFromValueFunction = [](double v)
        { return fractal_utils::normalizedToPercentString(static_cast<float>(v)); };
        wetMixSlider.valueFromTextFunction = [](const juce::String& text)
        { return static_cast<double>(fractal_utils::percentStringToNormalized(text)); };
    }

    constexpr int kRotaryTextBoxBelowHeightPx = 22;

    /**
     * Depois do FlexBox, reduz o rect do rotary para um quadrado útil + caixa de texto por baixo,
     * centrado na célula — evita o knob "esticado" quando a coluna é larga.
     */
    void tightenRotarySliderBounds(juce::Slider& rotarySlider)
    {
        auto currentBounds = rotarySlider.getBounds();
        const int rotaryDiameter = juce::jmin(currentBounds.getWidth() - 4,
                                                currentBounds.getHeight() - kRotaryTextBoxBelowHeightPx - 2);
        if (rotaryDiameter < 28)
            return;
        const int targetWidth = juce::jmax(rotaryDiameter + 4, kRotaryTextBoxWidthPx);
        const int targetHeight = rotaryDiameter + kRotaryTextBoxBelowHeightPx;
        rotarySlider.setBounds(currentBounds.withSizeKeepingCentre(juce::jmin(targetWidth, currentBounds.getWidth()),
                                                                   juce::jmin(targetHeight, currentBounds.getHeight())));
    }
} // namespace

/**
 * @brief Aplica estilo ao knob rotativo (agora usa StyleUtils::styleRotaryKnob).
 *
 * @details Mantida apenas para compatibilidade. Delega para a função centralizada.
 *
 * @note A largura da TextBox é ajustada depois em tightenRotarySliderBounds().
 */
void OutputPanel::styleRotary(juce::Slider& rotarySlider, bool interactive)
{
    GUI::styleRotaryKnob(rotarySlider, GUI::Colors::AccentPurple, interactive);
}

//==============================================================================
OutputPanel::MixColumn::MixColumn()
{
    mixLabel.setText("MIX", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setFont(juce::Font(juce::FontOptions(10.f)));
    mixLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe8eaed));
    addAndMakeVisible(mixLabel);

    OutputPanel::styleRotary(mixKnob, true);
    addAndMakeVisible(mixKnob);
}

void OutputPanel::MixColumn::resized()
{
    // Fluxo vertical: label + knob. stretch horizontal evita largura preferida 0 nos rotaries (invisíveis com "center").
    juce::FlexBox columnLayout;
    columnLayout.flexDirection = juce::FlexBox::Direction::column;
    columnLayout.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    columnLayout.alignItems = juce::FlexBox::AlignItems::stretch;
    columnLayout.items.add(juce::FlexItem(mixLabel).withHeight(14.f));
    columnLayout.items.add(juce::FlexItem(mixKnob).withFlex(1.f).withMinHeight(72.f));
    columnLayout.performLayout(getLocalBounds().toFloat());
    tightenRotarySliderBounds(mixKnob);
}

//==============================================================================
OutputPanel::GainLimiterColumn::GainLimiterColumn()
{
    gainLabel.setText("GAIN", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    gainLabel.setFont(juce::Font(juce::FontOptions(10.f)));
    gainLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe8eaed));
    addAndMakeVisible(gainLabel);

    OutputPanel::styleRotary(gainKnob, true);
    addAndMakeVisible(gainKnob);

    limiterLabel.setText("LIMITER", juce::dontSendNotification);
    limiterLabel.setJustificationType(juce::Justification::centred);
    limiterLabel.setFont(juce::Font(juce::FontOptions(9.f)));
    limiterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe8eaed));
    addAndMakeVisible(limiterLabel);

    addAndMakeVisible(limiterPill);
}

void OutputPanel::GainLimiterColumn::resized()
{
    juce::FlexBox columnLayout;
    columnLayout.flexDirection = juce::FlexBox::Direction::column;
    columnLayout.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    columnLayout.alignItems = juce::FlexBox::AlignItems::stretch;
    columnLayout.items.add(juce::FlexItem(gainLabel).withHeight(14.f));
    columnLayout.items.add(juce::FlexItem(gainKnob).withFlex(1.f).withMinHeight(52.f));
    columnLayout.items.add(juce::FlexItem(limiterLabel).withHeight(10.f));
    // O pill não deve esticar à largura total da coluna — só centrado, largura máx. 84 px.
    columnLayout.items.add(juce::FlexItem(limiterPill)
                                .withHeight(30.f)
                                .withMaxWidth(84.f)
                                .withAlignSelf(juce::FlexItem::AlignSelf::center));
    columnLayout.performLayout(getLocalBounds().toFloat());
    tightenRotarySliderBounds(gainKnob);
}

//==============================================================================
OutputPanel::ControlsRow::ControlsRow()
{
    addAndMakeVisible(mix);
    addAndMakeVisible(gain);
}

void OutputPanel::ControlsRow::resized()
{
    // Espaço entre a coluna MIX (flex) e a coluna GAIN+limiter (largura preferida com encolhimento).
    constexpr float kGapBetweenMixAndGainColumnsPx = 8.f;
    constexpr float kGainColumnPreferredWidthPx = 112.f;
    constexpr float kGainColumnMinWidthPx = 72.f;
    constexpr float kGainColumnMaxWidthPx = 112.f;

    juce::FlexBox controlRowLayout;
    controlRowLayout.flexDirection = juce::FlexBox::Direction::row;
    controlRowLayout.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    controlRowLayout.alignItems = juce::FlexBox::AlignItems::stretch;

    // withFlex(0,1, preferred): coluna GAIN tenta 112 px; flexShrink=1 permite encolher entre min/max se o painel apertar.
    controlRowLayout.items.add(juce::FlexItem(mix).withFlex(1.f, 1.f).withMinWidth(48.f));
    controlRowLayout.items.add(juce::FlexItem().withFlex(0.f).withWidth(kGapBetweenMixAndGainColumnsPx));
    controlRowLayout.items.add(juce::FlexItem(gain)
                                   .withFlex(0.f, 1.f, kGainColumnPreferredWidthPx)
                                   .withMinWidth(kGainColumnMinWidthPx)
                                   .withMaxWidth(kGainColumnMaxWidthPx));
    controlRowLayout.performLayout(getLocalBounds().toFloat());
}

//==============================================================================
OutputPanel::MeterCluster::MeterCluster()
{
    addAndMakeVisible(inMeter);
    addAndMakeVisible(outMeter);
    addAndMakeVisible(dbScaleRuler);

    inPeakReadout.setJustificationType(juce::Justification::centred);
    inPeakReadout.setFont(juce::Font(juce::FontOptions(10.f)));
    inPeakReadout.setColour(juce::Label::textColourId, juce::Colour(0xffe8eaed));
    inPeakReadout.setText("-inf", juce::dontSendNotification);
    addAndMakeVisible(inPeakReadout);

    outPeakReadout.setJustificationType(juce::Justification::centred);
    outPeakReadout.setFont(juce::Font(juce::FontOptions(10.f)));
    outPeakReadout.setColour(juce::Label::textColourId, juce::Colour(0xffe8eaed));
    outPeakReadout.setText("-inf", juce::dontSendNotification);
    addAndMakeVisible(outPeakReadout);
}

void OutputPanel::MeterCluster::resized()
{
    // Larguras fixas: colunas dos dois medidores + régua dB; a fila de picos por baixo replica a mesma geometria.
    constexpr float kFixedMeterColumnWidthPx = 40.f;
    constexpr float kDbRulerColumnWidthPx = 24.f;
    constexpr float kGapBetweenInAndOutMetersPx = 4.f;
    constexpr float kPeakReadoutsRowHeightPx = 16.f;

    auto panelBounds = getLocalBounds().toFloat();
    auto peakReadoutsRowBounds = panelBounds.removeFromBottom(kPeakReadoutsRowHeightPx);
    auto metersAndRulerRowBounds = panelBounds;

    {
        juce::FlexBox peakReadoutsRowLayout;
        peakReadoutsRowLayout.flexDirection = juce::FlexBox::Direction::row;
        peakReadoutsRowLayout.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        peakReadoutsRowLayout.alignItems = juce::FlexBox::AlignItems::stretch;
        peakReadoutsRowLayout.items.add(juce::FlexItem(inPeakReadout).withFlex(0.f).withWidth(kFixedMeterColumnWidthPx));
        peakReadoutsRowLayout.items.add(juce::FlexItem().withFlex(0.f).withWidth(kGapBetweenInAndOutMetersPx));
        peakReadoutsRowLayout.items.add(juce::FlexItem(outPeakReadout).withFlex(0.f).withWidth(kFixedMeterColumnWidthPx));
        // Item vazio: mesma largura que a régua, para o texto "-inf" ficar alinhado ao medidor por cima.
        peakReadoutsRowLayout.items.add(juce::FlexItem().withFlex(0.f).withWidth(kDbRulerColumnWidthPx));
        peakReadoutsRowLayout.performLayout(peakReadoutsRowBounds);
    }

    juce::FlexBox metersRowLayout;
    metersRowLayout.flexDirection = juce::FlexBox::Direction::row;
    metersRowLayout.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    metersRowLayout.alignItems = juce::FlexBox::AlignItems::stretch;
    metersRowLayout.items.add(juce::FlexItem(inMeter).withFlex(0.f).withWidth(kFixedMeterColumnWidthPx));
    metersRowLayout.items.add(juce::FlexItem().withFlex(0.f).withWidth(kGapBetweenInAndOutMetersPx));
    metersRowLayout.items.add(juce::FlexItem(outMeter).withFlex(0.f).withWidth(kFixedMeterColumnWidthPx));
    metersRowLayout.items.add(juce::FlexItem(dbScaleRuler).withFlex(0.f).withWidth(kDbRulerColumnWidthPx));
    metersRowLayout.performLayout(metersAndRulerRowBounds);
}

void OutputPanel::MeterCluster::updateLevels(float peakInLinear, float peakOutLinear)
{
    // Usa ConversionUtils para conversões padronizadas
    inMeter.setLevelDb(fractal_utils::linearToMeterDb(peakInLinear));
    outMeter.setLevelDb(fractal_utils::linearToMeterDb(peakOutLinear));
    inPeakReadout.setText(fractal_utils::formatPeakReadout(peakInLinear), juce::dontSendNotification);
    outPeakReadout.setText(fractal_utils::formatPeakReadout(peakOutLinear), juce::dontSendNotification);
}

//==============================================================================
OutputPanel::OutputPanel(FractalDistortionAudioProcessor& processor)
    : audioProcessor(processor)
{
    sectionTitle.setText("OUTPUT", juce::dontSendNotification);
    sectionTitle.setJustificationType(juce::Justification::centredLeft);
    sectionTitle.setFont(juce::Font(juce::FontOptions(12.f)).boldened());
    sectionTitle.setColour(juce::Label::textColourId, juce::Colour(kAccentColour));
    addAndMakeVisible(sectionTitle);

    addAndMakeVisible(controls);
    addAndMakeVisible(meters);

    // Sliders aninhados em `controls` — APVTS liga aos membros concretos, não ao painel.
    //mixKnobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    //    audioProcessor.getAPVTS(), "delayMix", controls.mix.mixKnob);
    //attachWetMixPercentDisplay(controls.mix.mixKnob);

    gainKnobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "outputGainDb", controls.gain.gainKnob);
}

void OutputPanel::paint(juce::Graphics& g)
{
    auto panelOutlineBounds = getLocalBounds().toFloat().reduced(2.f);
    g.setColour(juce::Colour(0xff1a1d26));
    g.fillRoundedRectangle(panelOutlineBounds, 6.f);
    g.setColour(juce::Colour(kPanelBorderColour));
    g.drawRoundedRectangle(panelOutlineBounds, 6.f, 1.f);

    if (meterDividerX > 0)
    {
        g.setColour(juce::Colour(kPanelBorderColour).withAlpha(0.9f));
        g.drawVerticalLine(meterDividerX,
                           panelOutlineBounds.getY() + kDividerLineTopInsetPx,
                           panelOutlineBounds.getBottom() - kDividerLineBottomInsetPx);
    }
}

void OutputPanel::updateOutputMetering(float peakInLinear, float peakOutLinear)
{
    meters.updateLevels(peakInLinear, peakOutLinear);
}

void OutputPanel::resized()
{
    meterDividerX = -1;
    // Margem interna comum ao painel antes de aplicar a grelha (título + duas colunas).
    auto paddedContentBounds = getLocalBounds().reduced(6);

    juce::Grid panelGrid;
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;
    using Px = juce::Grid::Px;

    panelGrid.templateRows = { Track(Px(18)), Track(Fr(1)) };
    // Fr só aceita inteiros nesta versão do JUCE; 100:48 ≈ proporção controlos : medidores.
    panelGrid.templateColumns = { Track(Fr(100)), Track(Fr(48)) };
    panelGrid.columnGap = Px(8);
    panelGrid.rowGap = Px(4);
    panelGrid.items = {
        juce::GridItem(sectionTitle).withArea(1, 1, 2, 3),
        juce::GridItem(controls).withArea(2, 1),
        juce::GridItem(meters).withArea(2, 2),
    };
    panelGrid.performLayout(paddedContentBounds);

    // Ponto médio visual entre a coluna de controlos e a de medidores (para a linha vertical decorativa).
    meterDividerX = (controls.getBounds().getRight() + meters.getBounds().getX()) / 2;

    // No tree, `meters` vem depois de `controls`; se houver 1 px de overlap por arredondamento, os rotaries ficam por cima.
    controls.toFront(false);

    repaint();
}

} // namespace GUI
