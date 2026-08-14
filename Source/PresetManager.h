#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace adaptivegate
{

/**
    Saves/loads the plugin's full APVTS state (every parameter, including Source)
    as named XML files under the user's application-data directory, the same way
    AdaptiveGateAudioProcessor::getStateInformation/setStateInformation already
    serialize state for the host - just to a named file instead of the host's own
    session data.
*/
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& stateToManage);

    void savePreset (const juce::String& presetName);
    void loadPreset (const juce::String& presetName);
    void deletePreset (const juce::String& presetName);

    /** Alphabetical. */
    juce::StringArray getAllPresetNames() const;

    static juce::File getPresetsDirectory();

private:
    juce::AudioProcessorValueTreeState& apvts;
};

} // namespace adaptivegate
