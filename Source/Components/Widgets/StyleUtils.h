#pragma once

#include <JuceHeader.h>

/**
 * @brief Utilitários de estilo visual para o plugin Fractal-Distortion.
 *
 * @details Centraliza:
 * - Paleta de cores do tema (baseada no mockup Figma)
 * - Funções de estilização de componentes (knobs, combos, labels)
 *
 * Elimina duplicação de código em DelayCorePanel, OutputPanel e outros.
 */
namespace GUI
{

/**
 * @brief Paleta de cores do tema Fractal-Distortion.
 *
 * @details Cores baseadas no mockup Figma. Todos os valores são uint32 ARGB.
 *
 * @note Usar estas constantes em vez de valores hex hardcoded garante:
 * - Consistência visual em todo o plugin
 * - Facilita mudanças globais de tema
 * - Torna o código mais legível (nomes semânticos)
 */
namespace Colors
{
    /** @brief Cor de fundo principal da janela do plugin. */
    constexpr juce::uint32 Background = 0xff0e1015;

    /** @brief Cor de fundo dos painéis (Delay Core, Character, Output). */
    constexpr juce::uint32 PanelBackground = 0xff1a1d26;

    /** @brief Cor de bordas, outlines e separadores. */
    constexpr juce::uint32 Border = 0xff3d4450;

    /** @brief Cor de fundo de inputs (TextBox, ComboBox). */
    constexpr juce::uint32 InputBackground = 0xff12151c;

    /** @brief Laranja vibrante - accent do Delay Core Panel. */
    constexpr juce::uint32 AccentOrange = 0xffff6b2d;

    /** @brief Roxo/lavanda - accent do Output Panel e medidores. */
    constexpr juce::uint32 AccentPurple = 0xffb388ff;

    /** @brief Texto primário (branco suave). */
    constexpr juce::uint32 TextPrimary = 0xffe8eaed;

    /** @brief Texto secundário (cinza claro). */
    constexpr juce::uint32 TextSecondary = 0xffc8d0dc;

    /** @brief Texto terciário/muted (cinza médio). */
    constexpr juce::uint32 TextMuted = 0xff9aa7b8;

    /** @brief Texto de labels escuros (usado em feedback, etc.). */
    constexpr juce::uint32 TextDark = 0xff6a7380;

} // namespace Colors

/**
 * @brief Aplica estilo padrão a um knob rotativo (Slider).
 *
 * @details Estilo visual:
 * - RotaryVerticalDrag (arrastar vertical para mudar valor)
 * - Arco de 1.15π a 2.85π (deixa gap inferior)
 * - TextBox abaixo do knob (72px × 22px)
 * - Cores customizáveis (accent, borda)
 *
 * @param slider       Slider a estilizar
 * @param accentColour Cor do preenchimento do knob (ex: AccentOrange, AccentPurple)
 * @param interactive  Se false, desabilita cliques (knobs decorativos)
 *
 * @note Este estilo é usado em DelayCorePanel, OutputPanel e CharacterPanel.
 *
 * @see DelayCorePanel::styleRotary (código original duplicado)
 * @see OutputPanel::styleRotary (código original duplicado)
 */
inline void styleRotaryKnob(juce::Slider& slider,
                            juce::uint32 accentColour = Colors::AccentOrange,
                            bool interactive = true)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.15f,
                               juce::MathConstants<float>::pi * 2.85f,
                               true);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 22);

    // Cores do knob
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(accentColour));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(Colors::Border));
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);

    // Cores da TextBox
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(Colors::InputBackground));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha(0.28f));

    slider.setInterceptsMouseClicks(interactive, interactive);
}

/**
 * @brief Aplica estilo padrão a um ComboBox (dropdown).
 *
 * @details Estilo visual:
 * - Fundo escuro (InputBackground)
 * - Texto branco
 * - Outline customizável (padrão: Border)
 *
 * @param combo         ComboBox a estilizar
 * @param outlineColour Cor da borda (opcional, padrão: Border)
 *
 * @note Usado em DelayCorePanel para MODE e SYNC combos.
 */
inline void styleComboBox(juce::ComboBox& combo, juce::uint32 outlineColour = Colors::Border)
{
    combo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(Colors::InputBackground));
    combo.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    combo.setColour(juce::ComboBox::outlineColourId, juce::Colour(outlineColour));
}

/**
 * @brief Aplica estilo a um Label de título de seção (ex: "DELAY CORE", "OUTPUT").
 *
 * @details Estilo visual:
 * - Fonte 11pt ou 12pt, bold
 * - Justificação centredLeft
 * - Cor: accent (laranja ou roxo)
 *
 * @param label        Label a estilizar
 * @param text         Texto do título
 * @param accentColour Cor do texto (padrão: AccentOrange)
 * @param fontSize     Tamanho da fonte (padrão: 11.0f)
 */
inline void styleSectionTitle(juce::Label& label,
                              const juce::String& text,
                              juce::uint32 accentColour = Colors::AccentOrange,
                              float fontSize = 11.0f)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setFont(juce::Font(juce::FontOptions(fontSize)).boldened());
    label.setColour(juce::Label::textColourId, juce::Colour(accentColour));
}

/**
 * @brief Aplica estilo a um Label de parâmetro pequeno (ex: "MODE", "SYNC", "MIX").
 *
 * @details Estilo visual:
 * - Fonte 9pt ou 10pt
 * - Justificação: customizável (padrão: centred)
 * - Cor: TextSecondary ou TextPrimary
 *
 * @param label         Label a estilizar
 * @param text          Texto do label
 * @param textColour    Cor do texto (padrão: TextSecondary)
 * @param fontSize      Tamanho da fonte (padrão: 10.0f)
 * @param justification Alinhamento (padrão: centred)
 */
inline void styleParameterLabel(juce::Label& label,
                                const juce::String& text,
                                juce::uint32 textColour = Colors::TextSecondary,
                                float fontSize = 10.0f,
                                juce::Justification justification = juce::Justification::centred)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(justification);
    label.setFont(juce::Font(juce::FontOptions(fontSize)));
    label.setColour(juce::Label::textColourId, juce::Colour(textColour));
}

/**
 * @brief Aplica estilo a um Label de pico/leitura (ex: "IN: -12.3 dB", "-inf").
 *
 * @details Estilo visual:
 * - Fonte 10pt ou 12pt
 * - Justificação: centred
 * - Cor: TextMuted ou TextPrimary
 *
 * @param label      Label a estilizar
 * @param initialText Texto inicial (será atualizado dinamicamente)
 * @param textColour  Cor do texto (padrão: TextMuted)
 * @param fontSize    Tamanho da fonte (padrão: 10.0f)
 */
inline void stylePeakLabel(juce::Label& label,
                           const juce::String& initialText,
                           juce::uint32 textColour = Colors::TextMuted,
                           float fontSize = 10.0f)
{
    label.setText(initialText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(fontSize)));
    label.setColour(juce::Label::textColourId, juce::Colour(textColour));
}

} // namespace GUI
