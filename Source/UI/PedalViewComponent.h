#pragma once

#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

/**
    Simplified "stompbox" view: the gate-pedal.png artwork as a background, with only the three
    controls a player needs live (Threshold, Sensitivity, Attack) exposed as knobs bound directly to
    the same apvts parameters the full Advanced view uses - no separate/derived parameters. Attack
    (not Mix) is the third knob: on a gate, how fast it opens is a core part of the sound, while
    dry/wet blending is a secondary, set-and-forget control better suited to the Advanced view.
    Source is pinned to "Guitar" whenever this view is visible (there's no Source control here, and
    the pedal skin is explicitly a guitar-pedal aesthetic); Range/Hold/Release/Hysteresis/Mix stay
    untouched at whatever value they currently hold.
*/
class PedalViewComponent final : public juce::Component
{
public:
    explicit PedalViewComponent (AdaptiveGateAudioProcessor&);
    ~PedalViewComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

    /** Set by the owning editor; invoked when the pedal view's own corner button is clicked. */
    std::function<void()> onSwitchToAdvanced;

private:
    void pinSourceToGuitar();

    AdaptiveGateAudioProcessor& processorRef;

    juce::Image backgroundImage;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    Knob thresholdKnob, sensitivityKnob, attackKnob;

    juce::TextButton advancedButton { "ADV" };

    std::unique_ptr<juce::LookAndFeel_V4> knobLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalViewComponent)
};
