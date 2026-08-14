#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/**
    Slim bar above the control list: a dropdown of saved presets (selecting one
    loads it immediately), plus Save/Delete buttons. Presets are the full APVTS
    state (every control, including Source) - see PresetManager.
*/
class PresetPanelComponent : public juce::Component,
                              private juce::ComboBox::Listener,
                              private juce::Button::Listener
{
public:
    explicit PresetPanelComponent (AdaptiveGateAudioProcessor&);
    ~PresetPanelComponent() override;

    void resized() override;

private:
    void comboBoxChanged (juce::ComboBox*) override;
    void buttonClicked (juce::Button*) override;

    void refreshPresetList();
    void promptAndSavePreset();

    AdaptiveGateAudioProcessor& processorRef;

    juce::ComboBox presetBox;
    juce::TextButton saveButton { "Save..." };
    juce::TextButton deleteButton { "Delete" };

    std::unique_ptr<juce::AlertWindow> saveDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetPanelComponent)
};
