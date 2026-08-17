#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace adaptivegate::ui
{
    class LookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();

        static const juce::Colour background, panel, cyan, cyanHot, violet, violetHot;
        static const juce::Colour text, textDim, threshold;
        static const juce::Identifier chromeProperty;

        void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,
                               juce::Slider::SliderStyle, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
        void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
        juce::Font getTextButtonFont (juce::TextButton&, int) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;

    private:
        juce::Image knob, rectOff, rectOn, sliderThumb;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LookAndFeel)
    };
}
