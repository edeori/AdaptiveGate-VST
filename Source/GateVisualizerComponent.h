#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/**
    Read-only state visualization sitting above the generic parameter editor:
    a one-line readout of the current control values (including the single
    global gate's resulting gain), plus a per-band detection view - each
    band's live envelope level against its own noise floor and gate-open
    threshold (noiseDb + resolved margin). The gate itself is global (see
    AdaptiveGateEngine), so every band's bar is coloured by the same shared
    gain; the band currently supplying the strongest evidence to that
    decision gets a bright border. Purely diagnostic - all editing still
    happens via the generic editor below it.
*/
class GateVisualizerComponent : public juce::Component,
                                 private juce::Timer
{
public:
    explicit GateVisualizerComponent (AdaptiveGateAudioProcessor&);
    ~GateVisualizerComponent() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    AdaptiveGateAudioProcessor& processorRef;

    // Timer-tick-smoothed copy of the engine's per-block metering snapshot, so the
    // ~30Hz redraw doesn't look like it's jumping between raw block-boundary values.
    struct SmoothedBand
    {
        float lowHz = 0.0f, highHz = 0.0f;
        float envelopeDb = -100.0f;
        float noiseDb = -100.0f;
        float thresholdDb = -100.0f;
        float gain = 0.0f;      // the single global gate's gain - same value in every band
        bool isDriving = false; // this band currently has the strongest excess (drives the global decision)
    };
    std::vector<SmoothedBand> smoothedBands;

    static juce::String formatHz (float hz);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateVisualizerComponent)
};
