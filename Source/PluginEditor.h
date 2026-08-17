#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GateVisualizerComponent.h"
#include "PresetPanelComponent.h"
#include "ControlPanelComponent.h"
#include "UI/AdaptiveGateLookAndFeel.h"
#include "UI/PedalViewComponent.h"

#if defined (JUCE_LAYOUT_TUNER) && JUCE_LAYOUT_TUNER
#include "Debug/JuceLayoutTuner.h"
#endif

class AdaptiveGateAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AdaptiveGateAudioProcessorEditor (AdaptiveGateAudioProcessor&);
    ~AdaptiveGateAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class ViewMode { Native, Pedal };

    void switchToView (ViewMode);

    adaptivegate::ui::LookAndFeel adaptiveLookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 500 };
    juce::Image backgroundImage, logoImage;

    ControlPanelComponent controlPanel;
    GateVisualizerComponent visualizer;
    PresetPanelComponent presetPanel;
    PedalViewComponent pedalView;

    juce::Component makerLayoutTarget, logoLayoutTarget, versionLayoutTarget;

    juce::TextButton pedalViewButton { "PEDAL VIEW" };
    ViewMode currentView = ViewMode::Pedal;

#if defined (JUCE_LAYOUT_TUNER) && JUCE_LAYOUT_TUNER
    std::unique_ptr<juce_layout_tuner::Overlay> layoutTuner;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveGateAudioProcessorEditor)
};
