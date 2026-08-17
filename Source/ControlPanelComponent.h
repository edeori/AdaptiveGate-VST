#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class ControlPanelComponent final : public juce::Component,
                                    private juce::Timer
{
public:
    explicit ControlPanelComponent (AdaptiveGateAudioProcessor&);
    ~ControlPanelComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setAdvancedVisible (bool);

    AdaptiveGateAudioProcessor& processorRef;

    juce::Label sourceLabel;
    juce::ComboBox sourceBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment;

    // Threshold, Sensitivity, Range, Attack, Hold, Release, Hysteresis, Mix.
    juce::OwnedArray<juce::Label> sliderLabels;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachments;

    juce::TextButton bypassButton { "BYPASS" };
    juce::TextButton advancedButton { "ADVANCED" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    bool advancedVisible = false;
    float inputDb = -66.0f;
    float outputDb = -66.0f;
    float reductionDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPanelComponent)
};
