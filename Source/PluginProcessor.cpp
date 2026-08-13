#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace adaptivegate;

AdaptiveGateAudioProcessor::AdaptiveGateAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AdaptiveGateAudioProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { sourceTypeParamId, 1 }, "Source",
        StringArray { "Speech", "Guitar", "Distorted Bass", "Drum (Close Mic)", "Drum (Overhead)" }, 1));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { thresholdParamId, 1 }, "Threshold", NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { sensitivityParamId, 1 }, "Sensitivity", NormalisableRange<float> (0.1f, 4.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { rangeParamId, 1 }, "Range", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { attackParamId, 1 }, "Attack", NormalisableRange<float> (0.1f, 4.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { holdParamId, 1 }, "Hold", NormalisableRange<float> (0.1f, 4.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { releaseParamId, 1 }, "Release", NormalisableRange<float> (0.1f, 4.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { hysteresisParamId, 1 }, "Hysteresis", NormalisableRange<float> (0.0f, 12.0f, 0.1f), 2.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { mixParamId, 1 }, "Mix", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { bypassParamId, 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

void AdaptiveGateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    engine.prepare (spec);
    lastSourceTypeIndex = -1;
    updateEngineParameters();
}

void AdaptiveGateAudioProcessor::releaseResources()
{
    engine.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AdaptiveGateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == out) && (in == mono || in == stereo);
}
#endif

void AdaptiveGateAudioProcessor::updateEngineParameters()
{
    const int sourceIndex = (int) *apvts.getRawParameterValue (sourceTypeParamId);
    if (sourceIndex != lastSourceTypeIndex)
    {
        lastSourceTypeIndex = sourceIndex;
        static const presets::SourceType map[] = {
            presets::SourceType::Speech,
            presets::SourceType::Guitar,
            presets::SourceType::Bass,
            presets::SourceType::DrumCloseMic,
            presets::SourceType::DrumOverhead
        };
        engine.setSourceProfile (map[(size_t) juce::jlimit (0, 4, sourceIndex)]);
    }

    engine.setThresholdOffsetDb (*apvts.getRawParameterValue (thresholdParamId));
    engine.setSensitivity (*apvts.getRawParameterValue (sensitivityParamId));
    engine.setMinGain (*apvts.getRawParameterValue (rangeParamId));
    engine.setAttackHoldReleaseMultiplier (*apvts.getRawParameterValue (attackParamId),
                                            *apvts.getRawParameterValue (holdParamId),
                                            *apvts.getRawParameterValue (releaseParamId));
    engine.setHysteresisDb (*apvts.getRawParameterValue (hysteresisParamId));
    engine.setMix (*apvts.getRawParameterValue (mixParamId));
    engine.setBypassed (*apvts.getRawParameterValue (bypassParamId) > 0.5f);
}

void AdaptiveGateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    updateEngineParameters();
    engine.process (buffer);
}

void AdaptiveGateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AdaptiveGateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* AdaptiveGateAudioProcessor::createEditor()
{
    return new AdaptiveGateAudioProcessorEditor (*this);
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdaptiveGateAudioProcessor();
}
