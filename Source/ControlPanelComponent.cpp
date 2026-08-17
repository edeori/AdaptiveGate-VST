#include "ControlPanelComponent.h"
#include "UI/AdaptiveGateFonts.h"
#include "UI/AdaptiveGateLookAndFeel.h"

namespace
{
    struct SliderSpec { const char* id; const char* label; const char* tooltip; };

    constexpr SliderSpec specs[] = {
        { AdaptiveGateAudioProcessor::thresholdParamId, "THRESHOLD", "Moves every band's detection margin. Higher is stricter." },
        { AdaptiveGateAudioProcessor::sensitivityParamId, "SENSITIVITY", "Sets how sharply the gate responds around the threshold." },
        { AdaptiveGateAudioProcessor::rangeParamId, "RANGE", "Sets the minimum gain while the gate is closed." },
        { AdaptiveGateAudioProcessor::attackParamId, "ATTACK", "Scales how quickly the gate opens." },
        { AdaptiveGateAudioProcessor::holdParamId, "HOLD", "Scales how long the gate stays open after a peak." },
        { AdaptiveGateAudioProcessor::releaseParamId, "RELEASE", "Scales how quickly the gate closes." },
        { AdaptiveGateAudioProcessor::hysteresisParamId, "HYSTERESIS", "Adds state-change stability around the threshold." },
        { AdaptiveGateAudioProcessor::mixParamId, "MIX", "Blends the dry and gated signals." },
    };

    juce::String formatPercent (double value) { return juce::String (juce::roundToInt (value * 100.0)) + "%"; }
    juce::String formatMultiplier (double value) { return juce::String (value, 2) + "x"; }
}

ControlPanelComponent::ControlPanelComponent (AdaptiveGateAudioProcessor& p)
    : processorRef (p)
{
    setInterceptsMouseClicks (false, true);

    sourceLabel.setText ("SOURCE", juce::dontSendNotification);
    sourceLabel.setFont (adaptivegate::ui::Fonts::medium (11.0f));
    sourceLabel.setColour (juce::Label::textColourId, adaptivegate::ui::LookAndFeel::textDim);
    addAndMakeVisible (sourceLabel);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter (AdaptiveGateAudioProcessor::sourceTypeParamId)))
    {
        int item = 1;
        for (const auto& name : choice->choices)
            sourceBox.addItem (name, item++);
    }
    sourceBox.setTooltip ("Selects the frequency-band profile used by the adaptive detector.");
    addAndMakeVisible (sourceBox);
    sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        p.apvts, AdaptiveGateAudioProcessor::sourceTypeParamId, sourceBox);

    for (int i = 0; i < (int) std::size (specs); ++i)
    {
        auto* label = sliderLabels.add (new juce::Label ({}, specs[i].label));
        label->setJustificationType (juce::Justification::centred);
        label->setFont (adaptivegate::ui::Fonts::medium (i == 0 ? 12.0f : 10.0f));
        label->setColour (juce::Label::textColourId, i == 0 ? adaptivegate::ui::LookAndFeel::cyan
                                                            : adaptivegate::ui::LookAndFeel::text);
        addAndMakeVisible (label);

        const bool threshold = i == 0;
        auto* slider = sliders.add (new juce::Slider (threshold ? juce::Slider::LinearHorizontal
                                                                : juce::Slider::RotaryHorizontalVerticalDrag,
                                                      threshold ? juce::Slider::TextBoxRight
                                                                : juce::Slider::TextBoxBelow));
        slider->setTooltip (specs[i].tooltip);
        const double defaultValue = i == 6 ? 2.0 : (i == 0 || i == 2) ? 0.0 : (i == 4 || i == 5) ? 0.3 : 1.0;
        slider->setDoubleClickReturnValue (true, defaultValue);
        slider->setNumDecimalPlacesToDisplay (i == 0 || i == 6 ? 1 : 2);

        if (i == 2 || i == 7)
            slider->textFromValueFunction = formatPercent;
        else if (i == 3 || i == 4 || i == 5)
            slider->textFromValueFunction = formatMultiplier;
        else if (i == 0 || i == 6)
            slider->textFromValueFunction = [] (double v) { return juce::String (v, 1) + " dB"; };

        addAndMakeVisible (slider);
        sliderAttachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, specs[i].id, *slider));

        // SliderAttachment installs the parameter's default formatter, so apply the compact
        // user-facing representation afterwards while keeping the parameter's native value range.
        if (i == 2 || i == 7)
        {
            slider->textFromValueFunction = formatPercent;
            slider->valueFromTextFunction = [] (const juce::String& text) { return text.retainCharacters ("0123456789.-").getDoubleValue() / 100.0; };
        }
        else if (i == 3 || i == 4 || i == 5)
            slider->textFromValueFunction = formatMultiplier;
        else if (i == 0 || i == 6)
            slider->textFromValueFunction = [] (double v) { return juce::String (v, 1) + " dB"; };
        slider->updateText();
    }

    bypassButton.setClickingTogglesState (true);
    bypassButton.getProperties().set (adaptivegate::ui::LookAndFeel::chromeProperty, true);
    bypassButton.setTooltip ("Passes the input through without processing.");
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        p.apvts, AdaptiveGateAudioProcessor::bypassParamId, bypassButton);

    advancedButton.setClickingTogglesState (true);
    advancedButton.getProperties().set (adaptivegate::ui::LookAndFeel::chromeProperty, true);
    advancedButton.onClick = [this] { setAdvancedVisible (advancedButton.getToggleState()); };
    addAndMakeVisible (advancedButton);

    setAdvancedVisible (false);
    startTimerHz (30);
}

ControlPanelComponent::~ControlPanelComponent()
{
    stopTimer();
}

void ControlPanelComponent::setAdvancedVisible (bool visible)
{
    advancedVisible = visible;
    advancedButton.setButtonText (visible ? "OVERVIEW" : "ADVANCED");
    sliderLabels[1]->setVisible (! visible);
    sliders[1]->setVisible (! visible);
    for (int i = 3; i <= 6; ++i)
    {
        sliderLabels[i]->setVisible (visible);
        sliders[i]->setVisible (visible);
    }
    repaint();
}

void ControlPanelComponent::timerCallback()
{
    const auto snapshot = processorRef.getMeterSnapshot();
    float nextInput = -66.0f;
    float gain = 1.0f;
    for (const auto& band : snapshot)
    {
        nextInput = juce::jmax (nextInput, band.envelopeDb);
        gain = band.gain;
    }
    inputDb += (nextInput - inputDb) * 0.28f;
    reductionDb = juce::Decibels::gainToDecibels (juce::jmax (gain, 0.001f));
    outputDb = juce::jmax (-66.0f, inputDb + reductionDb);
    repaint();
}

void ControlPanelComponent::paint (juce::Graphics& g)
{
    using L = adaptivegate::ui::LookAndFeel;
    const float sx = (float) getWidth() / 1100.0f;
    const float sy = (float) getHeight() / 720.0f;
    auto rect = [sx, sy] (float x, float y, float w, float h) { return juce::Rectangle<float> (x * sx, y * sy, w * sx, h * sy); };

    g.setColour (L::text);
    g.setFont (adaptivegate::ui::Fonts::title (14.0f * sy));
    g.drawText ("DETECTION", rect (820, 100, 260, 22), juce::Justification::centred);

    if (advancedVisible)
        return;

    auto drawMeter = [&] (const char* label, float db, float y)
    {
        g.setFont (adaptivegate::ui::Fonts::medium (9.0f * sy));
        g.setColour (L::textDim);
        g.drawText (label, rect (844, y, 58, 14), juce::Justification::centredLeft);
        auto track = rect (906, y + 4, 100, 5);
        g.setColour (juce::Colour (0xff151821));
        g.fillRoundedRectangle (track, 2.5f * sy);
        const float proportion = juce::jlimit (0.02f, 1.0f, (db + 66.0f) / 66.0f);
        g.setColour (L::cyanHot);
        g.fillRoundedRectangle (track.withWidth (track.getWidth() * proportion), 2.5f * sy);
        g.setColour (L::text);
        g.setFont (adaptivegate::ui::Fonts::numeric (10.0f * sy));
        g.drawText (juce::String (db, 1) + " dB", rect (1010, y, 48, 14), juce::Justification::centredRight);
    };

    drawMeter ("INPUT", inputDb, 400);
    drawMeter ("OUTPUT", outputDb, 424);
    drawMeter ("REDUCTION", reductionDb, 448);

    g.setColour (juce::Colour (0xff303640));
    g.drawRoundedRectangle (rect (844, 474, 212, 38), 5.0f * sy, 1.0f);
    g.setFont (adaptivegate::ui::Fonts::medium (9.0f * sy));
    g.setColour (L::textDim);
    g.drawText ("GLOBAL STATE", rect (856, 485, 100, 16), juce::Justification::centredLeft);
    g.setColour (L::cyanHot);
    g.setFont (adaptivegate::ui::Fonts::title (10.0f * sy));
    const auto state = reductionDb > -2.0f ? "OPEN" : (reductionDb > -18.0f ? "ADAPTING" : "CLOSED");
    g.drawText (state, rect (964, 485, 80, 16), juce::Justification::centredRight);
}

void ControlPanelComponent::resized()
{
    const float sx = (float) getWidth() / 1100.0f;
    const float sy = (float) getHeight() / 720.0f;
    auto bounds = [sx, sy] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (juce::roundToInt (x * sx), juce::roundToInt (y * sy),
                                     juce::roundToInt (w * sx), juce::roundToInt (h * sy));
    };

    bypassButton.setBounds (bounds (989, 23, 76, 30));
    sourceLabel.setBounds (bounds (844, 139, 212, 18));
    sourceBox.setBounds (bounds (844, 160, 212, 31));

    sliderLabels[0]->setBounds (bounds (48, 566, 120, 20));
    sliders[0]->setBounds (bounds (48, 584, 464, 48));

    sliderLabels[1]->setBounds (bounds (886, 331, 128, 18));
    sliders[1]->setBounds (bounds (878, 203, 144, 122));

    sliderLabels[2]->setBounds (bounds (530, 552, 64, 16));
    sliders[2]->setBounds (bounds (516, 567, 92, 77));
    sliderLabels[7]->setBounds (bounds (666, 552, 64, 16));
    sliders[7]->setBounds (bounds (652, 567, 92, 77));

    juce::Rectangle<int> advancedBounds[] = {
        bounds (842, 238, 92, 112), bounds (966, 238, 92, 112),
        bounds (842, 362, 92, 112), bounds (966, 362, 92, 112)
    };
    for (int i = 0; i < 4; ++i)
    {
        sliderLabels[i + 3]->setBounds (advancedBounds[i].removeFromBottom (18));
        sliders[i + 3]->setBounds (advancedBounds[i]);
    }

    advancedButton.setBounds (bounds (844, 596, 212, 31));
}
