#include "PluginEditor.h"

AdaptiveGateAudioProcessorEditor::AdaptiveGateAudioProcessorEditor (AdaptiveGateAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), genericEditor (p)
{
    addAndMakeVisible (genericEditor);
    setResizable (true, true);
    setSize (480, 480);
}

void AdaptiveGateAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AdaptiveGateAudioProcessorEditor::resized()
{
    genericEditor.setBounds (getLocalBounds());
}
