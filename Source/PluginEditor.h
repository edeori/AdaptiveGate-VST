#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GateVisualizerComponent.h"
#include "PresetPanelComponent.h"
#include "ControlPanelComponent.h"

/**
    Deliberately minimal editor: a custom per-band state visualizer, a preset bar,
    and a hand-built control list (so each control can carry a tooltip explaining
    what it does). Functional only, no custom look-and-feel -- skinning is
    explicitly out of scope for now.
*/
class AdaptiveGateAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AdaptiveGateAudioProcessorEditor (AdaptiveGateAudioProcessor&);
    ~AdaptiveGateAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AdaptiveGateAudioProcessor& processorRef;
    juce::TooltipWindow tooltipWindow { this, 500 };
    GateVisualizerComponent visualizer;
    PresetPanelComponent presetPanel;
    ControlPanelComponent controlPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveGateAudioProcessorEditor)
};
