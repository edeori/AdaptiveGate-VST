#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/**
    Deliberately minimal editor: JUCE's generic parameter list. Functional only,
    no custom look-and-feel -- polish is explicitly out of scope for now.
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
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveGateAudioProcessorEditor)
};
