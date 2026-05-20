#pragma once

#include <JuceHeader.h>

namespace GUI
{

/** Régua vertical de texto em dB (−60 … 0), alinhada ao mesmo mapeamento que os medidores segmentados. */
class DbScaleRuler : public juce::Component
{
public:
    DbScaleRuler() noexcept = default;

    void paint(juce::Graphics& g) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DbScaleRuler)
};

} // namespace GUI
