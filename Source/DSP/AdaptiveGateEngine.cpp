#include "AdaptiveGateEngine.h"

#include <algorithm>

namespace adaptivegate::dsp
{

void AdaptiveGateEngine::prepare (const juce::dsp::ProcessSpec& newSpec)
{
    spec = newSpec;

    filterBank.prepare (spec);
    filterBank.setCrossoverFrequencies (profile.getCrossoverFrequencies());

    scratchShapedGain.resize ((size_t) spec.maximumBlockSize);
    dryBuffer.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize, false, false, true);

    rebuildBandModules();
}

void AdaptiveGateEngine::setSourceProfile (presets::SourceType type)
{
    profile = presets::getProfileFor (type);
    if (spec.sampleRate > 0.0)
    {
        filterBank.setCrossoverFrequencies (profile.getCrossoverFrequencies());
        rebuildBandModules();
    }
}

void AdaptiveGateEngine::rebuildBandModules()
{
    const int numBands = filterBank.getNumBands();

    envelopeFollowers.assign ((size_t) numBands, EnvelopeFollower {});
    noiseTrackers.assign ((size_t) numBands, NoiseFloorTracker {});
    transientDetectors.assign ((size_t) numBands, TransientDetector {});
    gateDecisions.assign ((size_t) numBands, GateDecision {});
    gainEnvelopes.assign ((size_t) numBands, GainEnvelope {});
    lastBandGains.assign ((size_t) numBands, 0.0f);

    bandBuffers.resize ((size_t) numBands);
    for (auto& buf : bandBuffers)
        buf.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize, false, false, true);

    bandEnvDb.assign ((size_t) numBands, std::vector<float> ((size_t) spec.maximumBlockSize, -100.0f));
    bandNoiseDb.assign ((size_t) numBands, std::vector<float> ((size_t) spec.maximumBlockSize, -60.0f));
    bandSnrSnapshot.assign ((size_t) numBands, -60.0f);
    bandEnergySnapshot.assign ((size_t) numBands, -60.0f);

    for (int i = 0; i < numBands; ++i)
    {
        const auto& band = profile.bands[(size_t) std::min ((size_t) i, profile.bands.size() - 1)];

        envelopeFollowers[(size_t) i].prepare (spec.sampleRate, (int) spec.maximumBlockSize);
        envelopeFollowers[(size_t) i].setTimeConstants (1.0f, 15.0f);

        noiseTrackers[(size_t) i].prepare (spec.sampleRate, (int) spec.maximumBlockSize);
        noiseTrackers[(size_t) i].setAsymmetricTimes (50.0f, 4000.0f);
        noiseTrackers[(size_t) i].setMinStatsWindow (3.0f, 0.2f);

        transientDetectors[(size_t) i].prepare (spec.sampleRate, (int) spec.maximumBlockSize);
        transientDetectors[(size_t) i].setSensitivity (6.0f, 5.0f, 50.0f);

        gateDecisions[(size_t) i].prepare (spec.sampleRate, (int) spec.maximumBlockSize);
        const float gMin = minGainOverride >= 0.0f ? minGainOverride : 0.0f;
        gateDecisions[(size_t) i].setParameters (band.sigmoidK * sensitivityMultiplier, gMin);
        gateDecisions[(size_t) i].setHysteresis (hysteresisDb);

        gainEnvelopes[(size_t) i].prepare (spec.sampleRate, (int) spec.maximumBlockSize);
        gainEnvelopes[(size_t) i].setTimes (band.attackMs * attackMul, band.holdMs * holdMul, band.releaseMs * releaseMul);
    }
}

void AdaptiveGateEngine::setThresholdOffsetDb (float offsetDb) { thresholdOffsetDb = offsetDb; }

void AdaptiveGateEngine::setSensitivity (float sigmoidKMultiplier)
{
    sensitivityMultiplier = sigmoidKMultiplier;
    for (size_t i = 0; i < gateDecisions.size(); ++i)
    {
        const auto& band = profile.bands[std::min (i, profile.bands.size() - 1)];
        const float gMin = minGainOverride >= 0.0f ? minGainOverride : 0.0f;
        gateDecisions[i].setParameters (band.sigmoidK * sensitivityMultiplier, gMin);
    }
}

void AdaptiveGateEngine::setMinGain (float gMin01)
{
    minGainOverride = gMin01;
    for (size_t i = 0; i < gateDecisions.size(); ++i)
    {
        const auto& band = profile.bands[std::min (i, profile.bands.size() - 1)];
        gateDecisions[i].setParameters (band.sigmoidK * sensitivityMultiplier, gMin01);
    }
}

void AdaptiveGateEngine::setAttackHoldReleaseMultiplier (float aMul, float hMul, float rMul)
{
    attackMul = aMul; holdMul = hMul; releaseMul = rMul;
    for (size_t i = 0; i < gainEnvelopes.size(); ++i)
    {
        const auto& band = profile.bands[std::min (i, profile.bands.size() - 1)];
        gainEnvelopes[i].setTimes (band.attackMs * attackMul, band.holdMs * holdMul, band.releaseMs * releaseMul);
    }
}

void AdaptiveGateEngine::setHysteresisDb (float newHysteresisDb)
{
    hysteresisDb = newHysteresisDb;
    for (auto& gd : gateDecisions)
        gd.setHysteresis (hysteresisDb);
}

void AdaptiveGateEngine::setMix (float dryWet01) { mix = juce::jlimit (0.0f, 1.0f, dryWet01); }
void AdaptiveGateEngine::setBypassed (bool shouldBypass) { bypassed = shouldBypass; }

void AdaptiveGateEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed || spec.sampleRate <= 0.0)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numBands = filterBank.getNumBands();
    if (numBands == 0 || numSamples == 0)
        return;

    if (mix < 1.0f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
    }

    filterBank.process (buffer, bandBuffers);

    // Pass 1: run each band's (stateful!) EnvelopeFollower and NoiseFloorTracker
    // exactly once for this block, caching the full per-sample dB arrays in
    // bandEnvDb/bandNoiseDb. These modules carry one-pole filter state and
    // ring-buffer history across calls, so calling process() more than once
    // per block per band would double-advance that state - the per-sample
    // gate pass below re-reads the cached arrays instead of re-processing.
    for (int b = 0; b < numBands; ++b)
    {
        envelopeFollowers[(size_t) b].process (bandBuffers[(size_t) b], bandEnvDb[(size_t) b].data(), numSamples);
        noiseTrackers[(size_t) b].process (bandEnvDb[(size_t) b].data(), bandNoiseDb[(size_t) b].data(), numSamples);

        bandEnergySnapshot[(size_t) b] = bandEnvDb[(size_t) b][(size_t) numSamples - 1];
        bandSnrSnapshot[(size_t) b] = bandEnvDb[(size_t) b][(size_t) numSamples - 1] - bandNoiseDb[(size_t) b][(size_t) numSamples - 1];
    }

    // Macro coherence / bass-control decision, from the block-level snapshot above.
    const auto marginAdjustments = presets::computeAllMarginAdjustments (profile, bandSnrSnapshot, bandEnergySnapshot);

    buffer.clear();

    // Pass 2: per-sample gate decision + gain shaping, reading the cached
    // envelope/noise arrays from pass 1 (TransientDetector is fed here, once,
    // since its per-sample onset/decay state must also only advance once per block).
    for (int b = 0; b < numBands; ++b)
    {
        const auto& band = profile.bands[(size_t) std::min ((size_t) b, profile.bands.size() - 1)];
        const float margin = band.resolveMarginDb (profile.marginWeightConstant)
                              + thresholdOffsetDb
                              + marginAdjustments[(size_t) b];

        const auto& envDb = bandEnvDb[(size_t) b];
        const auto& noiseDb = bandNoiseDb[(size_t) b];

        for (int n = 0; n < numSamples; ++n)
        {
            const float snr = envDb[(size_t) n] - noiseDb[(size_t) n];
            transientDetectors[(size_t) b].processSample (envDb[(size_t) n]);
            const float boost = transientDetectors[(size_t) b].getTransientBoost();

            const float target = gateDecisions[(size_t) b].computeGain (snr, margin, boost);
            scratchShapedGain[(size_t) n] = gainEnvelopes[(size_t) b].processSample (target);
        }

        lastBandGains[(size_t) b] = scratchShapedGain[(size_t) (numSamples - 1)];

        auto& bandBuf = bandBuffers[(size_t) b];
        for (int ch = 0; ch < bandBuf.getNumChannels() && ch < buffer.getNumChannels(); ++ch)
        {
            auto* bandData = bandBuf.getWritePointer (ch);
            for (int n = 0; n < numSamples; ++n)
                bandData[n] *= scratchShapedGain[(size_t) n];

            buffer.addFrom (ch, 0, bandBuf, ch, 0, numSamples);
        }
    }

    if (mix < 1.0f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer (ch);
            auto* dry = dryBuffer.getReadPointer (ch);
            for (int n = 0; n < numSamples; ++n)
                wet[n] = dry[n] * (1.0f - mix) + wet[n] * mix;
        }
    }
}

void AdaptiveGateEngine::reset()
{
    filterBank.reset();
    for (auto& e : envelopeFollowers) e.reset();
    for (auto& n : noiseTrackers) n.reset();
    for (auto& t : transientDetectors) t.reset();
    for (auto& g : gateDecisions) g.reset();
    for (auto& g : gainEnvelopes) g.reset();
}

} // namespace adaptivegate::dsp
