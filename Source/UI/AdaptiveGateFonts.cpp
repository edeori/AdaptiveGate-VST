#include "AdaptiveGateFonts.h"
#include "BinaryData.h"

namespace
{
    juce::Font makeFont (const void* data, int size, float height)
    {
        auto typeface = juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        return juce::Font (juce::FontOptions (typeface)).withHeight (height);
    }
}

namespace adaptivegate::ui::Fonts
{
    juce::Font title (float h)   { return makeFont (BinaryData::OxaniumBold_ttf, BinaryData::OxaniumBold_ttfSize, h); }
    juce::Font light (float h)   { return makeFont (BinaryData::BarlowCondensedLight_ttf, BinaryData::BarlowCondensedLight_ttfSize, h); }
    juce::Font medium (float h)  { return makeFont (BinaryData::BarlowCondensedMedium_ttf, BinaryData::BarlowCondensedMedium_ttfSize, h); }
    juce::Font numeric (float h) { return makeFont (BinaryData::RajdhaniMedium_ttf, BinaryData::RajdhaniMedium_ttfSize, h); }
}
