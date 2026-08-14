#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/**
    Hand-built replacement for JUCE's GenericAudioProcessorEditor: functionally the
    same (one row per parameter, using APVTS attachments so no manual sync code is
    needed), but each control carries a real tooltip explaining what it does.
    GenericAudioProcessorEditor's internal row components aren't a stable API to
    retrofit tooltips onto, hence building this by hand instead. Still no custom
    look-and-feel/skin - purely functional.
*/
class ControlPanelComponent : public juce::Component
{
public:
    explicit ControlPanelComponent (AdaptiveGateAudioProcessor&);

    void resized() override;

private:
    juce::Label sourceLabel;
    juce::ComboBox sourceBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment;

    // Parallel arrays, one entry per float/slider parameter (Threshold, Sensitivity,
    // Range, Attack, Hold, Release, Hysteresis, Mix) - built in a loop in the .cpp
    // rather than as 8 near-identical named members.
    juce::OwnedArray<juce::Label> sliderLabels;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachments;

    juce::ToggleButton bypassButton { "Bypass" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPanelComponent)
};
