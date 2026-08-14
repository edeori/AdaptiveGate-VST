#include "PluginEditor.h"

AdaptiveGateAudioProcessorEditor::AdaptiveGateAudioProcessorEditor (AdaptiveGateAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), visualizer (p), presetPanel (p), controlPanel (p)
{
    addAndMakeVisible (visualizer);
    addAndMakeVisible (presetPanel);
    addAndMakeVisible (controlPanel);
    setResizable (true, true);
    setSize (520, 700);
}

void AdaptiveGateAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AdaptiveGateAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    visualizer.setBounds (area.removeFromTop (220));
    presetPanel.setBounds (area.removeFromTop (34));
    controlPanel.setBounds (area);
}
