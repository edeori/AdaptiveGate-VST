#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/AdaptiveGateEngine.h"

class AdaptiveGateAudioProcessor : public juce::AudioProcessor
{
public:
    AdaptiveGateAudioProcessor();
    ~AdaptiveGateAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Parameter IDs (shared with the editor)
    static constexpr const char* sourceTypeParamId = "sourceType";
    static constexpr const char* thresholdParamId = "threshold";
    static constexpr const char* sensitivityParamId = "sensitivity";
    static constexpr const char* rangeParamId = "range";
    static constexpr const char* attackParamId = "attack";
    static constexpr const char* holdParamId = "hold";
    static constexpr const char* releaseParamId = "release";
    static constexpr const char* hysteresisParamId = "hysteresis";
    static constexpr const char* mixParamId = "mix";
    static constexpr const char* bypassParamId = "bypass";

private:
    void updateEngineParameters();

    adaptivegate::dsp::AdaptiveGateEngine engine;
    int lastSourceTypeIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveGateAudioProcessor)
};
