#include "ControlPanelComponent.h"

namespace
{
    struct SliderSpec
    {
        const char* paramId;
        const char* labelText;
        const char* tooltip;
    };

    const SliderSpec sliderSpecs[] = {
        { AdaptiveGateAudioProcessor::thresholdParamId, "Threshold",
          "Offset added to every band's detection margin. Higher = stricter (gate opens less easily); lower = more permissive." },
        { AdaptiveGateAudioProcessor::sensitivityParamId, "Sensitivity",
          "Multiplies the global gate's sigmoid steepness. Higher = snappier, more switch-like open/close transitions." },
        { AdaptiveGateAudioProcessor::rangeParamId, "Range",
          "Minimum gain floor when closed (linear 0-1, not dB). 0 = full mute; higher values mean the gate never fully closes." },
        { AdaptiveGateAudioProcessor::attackParamId, "Attack",
          "Multiplies the driving band's attack time - how fast the gate opens when signal appears." },
        { AdaptiveGateAudioProcessor::holdParamId, "Hold",
          "Multiplies the driving band's hold time - how long the gate stays open after a peak before it's allowed to release." },
        { AdaptiveGateAudioProcessor::releaseParamId, "Release",
          "Multiplies the driving band's release time - how fast the gate closes after the hold period ends." },
        { AdaptiveGateAudioProcessor::hysteresisParamId, "Hysteresis",
          "Extra dB the driving band's SNR must cross to flip the gate's state, so it doesn't chatter near the threshold." },
        { AdaptiveGateAudioProcessor::mixParamId, "Mix",
          "Dry/Wet blend between the unprocessed and gated signal. 0 = dry, 1 = fully gated." },
    };
}

ControlPanelComponent::ControlPanelComponent (AdaptiveGateAudioProcessor& p)
{
    sourceLabel.setText ("Source", juce::dontSendNotification);
    addAndMakeVisible (sourceLabel);

    sourceBox.setTooltip ("Frequency-band profile matched to the input material: sets crossover points and each "
                           "band's detection margin/timing for when it drives the global gate.");
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter (AdaptiveGateAudioProcessor::sourceTypeParamId)))
    {
        int itemId = 1;
        for (auto& choice : choiceParam->choices)
            sourceBox.addItem (choice, itemId++);
    }
    addAndMakeVisible (sourceBox);
    sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        p.apvts, AdaptiveGateAudioProcessor::sourceTypeParamId, sourceBox);

    for (const auto& spec : sliderSpecs)
    {
        auto* label = sliderLabels.add (new juce::Label ({}, spec.labelText));
        addAndMakeVisible (label);

        auto* slider = sliders.add (new juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight));
        slider->setTooltip (spec.tooltip);
        addAndMakeVisible (slider);

        sliderAttachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, spec.paramId, *slider));
    }

    bypassButton.setTooltip ("Disables all processing - the signal passes through unchanged.");
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        p.apvts, AdaptiveGateAudioProcessor::bypassParamId, bypassButton);
}

void ControlPanelComponent::resized()
{
    constexpr int labelWidth = 110;
    constexpr int rowMargin = 6;

    auto area = getLocalBounds().reduced (8);
    const int numRows = 1 + sliders.size() + 1; // source + sliders + bypass
    const int rowHeight = numRows > 0 ? area.getHeight() / numRows : 0;

    auto takeRow = [&] { return area.removeFromTop (rowHeight).reduced (0, rowMargin / 2); };

    {
        auto row = takeRow();
        sourceLabel.setBounds (row.removeFromLeft (labelWidth));
        sourceBox.setBounds (row);
    }

    for (int i = 0; i < sliders.size(); ++i)
    {
        auto row = takeRow();
        sliderLabels[i]->setBounds (row.removeFromLeft (labelWidth));
        sliders[i]->setBounds (row);
    }

    {
        auto row = takeRow();
        bypassButton.setBounds (row.removeFromLeft (labelWidth + 100));
    }
}
