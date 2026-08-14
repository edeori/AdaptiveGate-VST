#include "GateVisualizerComponent.h"

namespace
{
    constexpr float meterMinDb = -66.0f;
    constexpr float meterMaxDb = 0.0f;

    float dbToY (float db, juce::Rectangle<float> area)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (db - meterMinDb) / (meterMaxDb - meterMinDb));
        return area.getBottom() - t * area.getHeight();
    }
}

GateVisualizerComponent::GateVisualizerComponent (AdaptiveGateAudioProcessor& p)
    : processorRef (p)
{
    startTimerHz (30);
}

GateVisualizerComponent::~GateVisualizerComponent()
{
    stopTimer();
}

juce::String GateVisualizerComponent::formatHz (float hz)
{
    if (hz >= 1000.0f)
        return juce::String (hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + "k";
    return juce::String ((int) hz);
}

void GateVisualizerComponent::timerCallback()
{
    const auto snapshot = processorRef.getMeterSnapshot();

    if (smoothedBands.size() != snapshot.size())
        smoothedBands.assign (snapshot.size(), SmoothedBand {});

    constexpr float smoothing = 0.35f; // toward-target factor per ~33ms tick
    for (size_t i = 0; i < snapshot.size(); ++i)
    {
        auto& s = smoothedBands[i];
        const auto& t = snapshot[i];
        s.lowHz = t.lowHz;
        s.highHz = t.highHz;
        s.envelopeDb += (t.envelopeDb - s.envelopeDb) * smoothing;
        s.noiseDb += (t.noiseDb - s.noiseDb) * smoothing;
        s.thresholdDb += (t.thresholdDb - s.thresholdDb) * smoothing;
        s.gain += (t.gain - s.gain) * smoothing;
        s.isDriving = t.isDriving;
    }

    repaint();
}

void GateVisualizerComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (bounds);

    // --- Current control-value readout ----------------------------------
    auto readoutArea = bounds.removeFromTop (26.0f).toNearestInt();
    auto& apvts = processorRef.apvts;

    const bool bypassed = apvts.getRawParameterValue (AdaptiveGateAudioProcessor::bypassParamId)->load() > 0.5f;

    const float globalGain = smoothedBands.empty() ? 0.0f : smoothedBands.front().gain;

    juce::String readout;
    readout << apvts.getParameter (AdaptiveGateAudioProcessor::sourceTypeParamId)->getCurrentValueAsText()
            << "   Gate " << juce::String ((int) (globalGain * 100.0f)) << "%"
            << "   Thr " << juce::String (apvts.getRawParameterValue (AdaptiveGateAudioProcessor::thresholdParamId)->load(), 1) << "dB"
            << "   Sens " << juce::String (apvts.getRawParameterValue (AdaptiveGateAudioProcessor::sensitivityParamId)->load(), 2)
            << "   Range " << juce::String (apvts.getRawParameterValue (AdaptiveGateAudioProcessor::rangeParamId)->load(), 2)
            << "   A/H/R " << juce::String (apvts.getRawParameterValue (AdaptiveGateAudioProcessor::attackParamId)->load(), 2) << "x/"
                            << juce::String (apvts.getRawParameterValue (AdaptiveGateAudioProcessor::holdParamId)->load(), 2) << "x/"
                            << juce::String (apvts.getRawParameterValue (AdaptiveGateAudioProcessor::releaseParamId)->load(), 2) << "x"
            << "   Hyst " << juce::String (apvts.getRawParameterValue (AdaptiveGateAudioProcessor::hysteresisParamId)->load(), 1) << "dB"
            << "   Mix " << (int) (apvts.getRawParameterValue (AdaptiveGateAudioProcessor::mixParamId)->load() * 100.0f) << "%";
    if (bypassed)
        readout << "   [BYPASSED]";

    g.setColour (bypassed ? juce::Colours::orange : juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawFittedText (readout, readoutArea, juce::Justification::centredLeft, 1);

    if (smoothedBands.empty())
        return;

    bounds.reduce (6.0f, 4.0f);
    auto labelArea = bounds.removeFromBottom (16.0f);

    const float bandWidth = bounds.getWidth() / (float) smoothedBands.size();

    for (size_t i = 0; i < smoothedBands.size(); ++i)
    {
        const auto& b = smoothedBands[i];
        const juce::Rectangle<float> col (bounds.getX() + (float) i * bandWidth, bounds.getY(), bandWidth, bounds.getHeight());
        const auto inner = col.reduced (3.0f, 0.0f);

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRect (inner);

        // Envelope bar, filled from the bottom up to the current level, coloured by the
        // (single, global) gate gain. Fixed hue (cyan), ramping saturation/brightness
        // from dim grey (closed) to vivid (open) - deliberately NOT a red/green hue
        // sweep, which is unreadable for red-green colour blindness.
        const float g01 = juce::jlimit (0.0f, 1.0f, b.gain);
        const auto gainColour = juce::Colour::fromHSV (0.52f, 0.15f + 0.55f * g01, 0.30f + 0.60f * g01, 1.0f);
        g.setColour (gainColour);
        g.fillRect (inner.withTop (dbToY (b.envelopeDb, inner)));

        // Noise floor tick (grey)
        const float noiseY = dbToY (b.noiseDb, inner);
        g.setColour (juce::Colours::lightgrey.withAlpha (0.8f));
        g.drawLine (inner.getX(), noiseY, inner.getRight(), noiseY, 1.0f);

        // Gate-open threshold tick (amber) - the level this band would need to clear
        // to be the one arguing "open" for the shared, global gate.
        const float threshY = dbToY (b.thresholdDb, inner);
        g.setColour (juce::Colours::orange.withAlpha (0.9f));
        g.drawLine (inner.getX(), threshY, inner.getRight(), threshY, 1.5f);

        // Driving band (currently supplying the strongest evidence to the global
        // gate) gets a bright border instead of the usual faint one.
        g.setColour (b.isDriving ? juce::Colours::white.withAlpha (0.9f) : juce::Colours::white.withAlpha (0.15f));
        g.drawRect (inner, b.isDriving ? 2.0f : 1.0f);

        const auto labCol = juce::Rectangle<float> (labelArea.getX() + (float) i * bandWidth, labelArea.getY(), bandWidth, labelArea.getHeight());
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        g.drawFittedText (formatHz (b.lowHz) + "-" + formatHz (b.highHz), labCol.toNearestInt(), juce::Justification::centred, 1);
    }
}
