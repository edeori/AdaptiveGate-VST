#include "AdaptiveGateEngine.h"

#include <algorithm>

namespace adaptivegate::dsp
{

void AdaptiveGateEngine::prepare (const juce::dsp::ProcessSpec& newSpec)
{
    spec = newSpec;

    filterBank.prepare (spec);
    filterBank.setCrossoverFrequencies (profile.getCrossoverFrequencies());

    scratchGlobalGain.resize ((size_t) spec.maximumBlockSize);
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

    bandBuffers.resize ((size_t) numBands);
    for (auto& buf : bandBuffers)
        buf.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize, false, false, true);

    bandEnvDb.assign ((size_t) numBands, std::vector<float> ((size_t) spec.maximumBlockSize, -100.0f));
    bandNoiseDb.assign ((size_t) numBands, std::vector<float> ((size_t) spec.maximumBlockSize, -60.0f));
    bandMarginDb.assign ((size_t) numBands, 0.0f);
    bandSnrSnapshot.assign ((size_t) numBands, -60.0f);
    bandEnergySnapshot.assign ((size_t) numBands, -60.0f);

    meterScratch.assign ((size_t) numBands, BandMeter {});
    {
        const juce::SpinLock::ScopedLockType sl (meterLock);
        meterSnapshot.assign ((size_t) numBands, BandMeter {});
    }

    float sigmoidKSum = 0.0f;
    for (int i = 0; i < numBands; ++i)
    {
        const auto& band = profile.bands[(size_t) std::min ((size_t) i, profile.bands.size() - 1)];
        sigmoidKSum += band.sigmoidK;

        envelopeFollowers[(size_t) i].prepare (spec.sampleRate, (int) spec.maximumBlockSize);
        envelopeFollowers[(size_t) i].setTimeConstants (1.0f, 15.0f);

        noiseTrackers[(size_t) i].prepare (spec.sampleRate, (int) spec.maximumBlockSize);
        noiseTrackers[(size_t) i].setAsymmetricTimes (50.0f, 4000.0f);
        noiseTrackers[(size_t) i].setMinStatsWindow (3.0f, 0.2f);

        transientDetectors[(size_t) i].prepare (spec.sampleRate, (int) spec.maximumBlockSize);
        transientDetectors[(size_t) i].setSensitivity (6.0f, 5.0f, 50.0f);

        meterScratch[(size_t) i].lowHz = band.lowHz;
        meterScratch[(size_t) i].highHz = band.highHz;
    }

    // Representative steepness for the single global gate: the per-band sigmoidK
    // values in FrequencyProfile still express "how snappy should the decision be
    // if THIS band is the one providing the evidence" - averaging keeps a flavor of
    // that tuning without needing a discontinuous per-sample k switch (unlike
    // attack/hold/release, switching k while the sigmoid is mid-evaluation would
    // visibly kink the probability curve, not just its time constant).
    globalBaseSigmoidK = numBands > 0 ? sigmoidKSum / (float) numBands : 0.5f;

    globalGateDecision.prepare (spec.sampleRate, (int) spec.maximumBlockSize);
    globalGainEnvelope.prepare (spec.sampleRate, (int) spec.maximumBlockSize);
    globalGateDecision.setHysteresis (hysteresisDb);
    updateGlobalGateParameters();
    lastDrivingBandIndex = -1; // force a setTimes() on the first sample processed after this rebuild
}

void AdaptiveGateEngine::updateGlobalGateParameters()
{
    const float gMin = minGainOverride >= 0.0f ? minGainOverride : 0.0f;
    globalGateDecision.setParameters (globalBaseSigmoidK * sensitivityMultiplier, gMin);
}

void AdaptiveGateEngine::setThresholdOffsetDb (float offsetDb) { thresholdOffsetDb = offsetDb; }

void AdaptiveGateEngine::setSensitivity (float sigmoidKMultiplier)
{
    sensitivityMultiplier = sigmoidKMultiplier;
    updateGlobalGateParameters();
}

void AdaptiveGateEngine::setMinGain (float gMin01)
{
    minGainOverride = gMin01;
    updateGlobalGateParameters();
}

void AdaptiveGateEngine::setAttackHoldReleaseMultiplier (float aMul, float hMul, float rMul)
{
    attackMul = aMul; holdMul = hMul; releaseMul = rMul;
    // Actual times are (re)applied per-sample in process(), following whichever
    // band is currently driving the global decision - no per-band loop needed here.
}

void AdaptiveGateEngine::setHysteresisDb (float newHysteresisDb)
{
    hysteresisDb = newHysteresisDb;
    globalGateDecision.setHysteresis (hysteresisDb);
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

    // FilterBank's split is used ONLY to feed the per-band analysis modules below -
    // the bands are never re-summed into the output (see class comment in the header).
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

    // Resolved per-band margin for this block (constant across the block's samples).
    for (int b = 0; b < numBands; ++b)
    {
        const auto& band = profile.bands[(size_t) std::min ((size_t) b, profile.bands.size() - 1)];
        bandMarginDb[(size_t) b] = band.resolveMarginDb (profile.marginWeightConstant)
                                    + thresholdOffsetDb
                                    + marginAdjustments[(size_t) b];
    }

    // Pass 2: per-sample, find whichever band shows the strongest evidence of real
    // signal - "excess" = SNR above that band's resolved margin, in dB - and let
    // THAT single value (plus the loudest transient boost from any band) drive one
    // global gate. TransientDetector is fed here, once, since its per-sample
    // onset/decay state must only advance once per block.
    int drivingBand = lastDrivingBandIndex;
    for (int n = 0; n < numSamples; ++n)
    {
        float bestExcess = -1000.0f;
        float maxTransientBoost = 0.0f;
        int bestBandIdx = 0;

        for (int b = 0; b < numBands; ++b)
        {
            const float envDbN = bandEnvDb[(size_t) b][(size_t) n];
            const float noiseDbN = bandNoiseDb[(size_t) b][(size_t) n];

            transientDetectors[(size_t) b].processSample (envDbN);
            maxTransientBoost = std::max (maxTransientBoost, transientDetectors[(size_t) b].getTransientBoost());

            const float excess = (envDbN - noiseDbN) - bandMarginDb[(size_t) b];
            if (excess > bestExcess)
            {
                bestExcess = excess;
                bestBandIdx = b;
            }

            if (n == numSamples - 1)
            {
                auto& meter = meterScratch[(size_t) b];
                meter.envelopeDb = envDbN;
                meter.noiseDb = noiseDbN;
                meter.thresholdDb = noiseDbN + bandMarginDb[(size_t) b];
            }
        }

        if (bestBandIdx != drivingBand)
        {
            const auto& wb = profile.bands[(size_t) std::min ((size_t) bestBandIdx, profile.bands.size() - 1)];
            globalGainEnvelope.setTimes (wb.attackMs * attackMul, wb.holdMs * holdMul, wb.releaseMs * releaseMul);
            drivingBand = bestBandIdx;
        }

        const float target = globalGateDecision.computeGain (bestExcess, 0.0f, maxTransientBoost);
        scratchGlobalGain[(size_t) n] = globalGainEnvelope.processSample (target);
    }
    lastDrivingBandIndex = drivingBand;

    // Apply the single global gain directly to the original (unfiltered) signal.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int n = 0; n < numSamples; ++n)
            data[n] *= scratchGlobalGain[(size_t) n];
    }

    const float lastGain = scratchGlobalGain[(size_t) (numSamples - 1)];
    for (int b = 0; b < numBands; ++b)
    {
        meterScratch[(size_t) b].gain = lastGain;
        meterScratch[(size_t) b].isDriving = (b == drivingBand);
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

    {
        const juce::SpinLock::ScopedLockType sl (meterLock);
        meterSnapshot = meterScratch;
    }
}

void AdaptiveGateEngine::reset()
{
    filterBank.reset();
    for (auto& e : envelopeFollowers) e.reset();
    for (auto& n : noiseTrackers) n.reset();
    for (auto& t : transientDetectors) t.reset();
    globalGateDecision.reset();
    globalGainEnvelope.reset();
}

} // namespace adaptivegate::dsp
