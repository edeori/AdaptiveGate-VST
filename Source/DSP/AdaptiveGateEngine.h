#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

#include "FilterBank.h"
#include "EnvelopeFollower.h"
#include "NoiseFloorTracker.h"
#include "TransientDetector.h"
#include "GateDecision.h"
#include "GainEnvelope.h"
#include "../Presets/FrequencyProfile.h"
#include "../Presets/Coherence.h"

namespace adaptivegate::dsp
{

/**
    Top-level orchestrator wiring the full architecture from docs/description.md:

        Input -> FilterBank -> {EnvelopeFollower, NoiseFloorTracker, TransientDetector}
               -> per-band SNR -> margin adjustments (weighting/coherence/bass-control)
               -> GateDecision (sigmoid probability -> soft gain)
               -> GainEnvelope (attack/hold/release)
               -> re-sum bands -> Output (dry/wet mix)

    This is the integration point all the individual DSP modules are implemented
    against; it owns one instance of each per-band module and one FilterBank.
*/
class AdaptiveGateEngine
{
public:
    AdaptiveGateEngine() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);

    /** Switches source-adaptive preset; rebuilds the filterbank and per-band module arrays. */
    void setSourceProfile (presets::SourceType type);

    // --- User-facing parameters (applied on top of the active profile's per-band values) ---
    void setThresholdOffsetDb (float offsetDb);   // added to every band's resolved margin
    void setSensitivity (float sigmoidKMultiplier); // multiplies each band's sigmoidK
    void setMinGain (float gMin01);                 // overrides Gmin (0..1) for all bands
    void setAttackHoldReleaseMultiplier (float attackMul, float holdMul, float releaseMul);
    void setHysteresisDb (float hysteresisDb);
    void setMix (float dryWet01);                    // 0 = dry, 1 = fully gated
    void setBypassed (bool shouldBypass);

    /** Processes in place; buffer channel count/size must match what prepare() was called with (or less). */
    void process (juce::AudioBuffer<float>& buffer);

    void reset();

    int getNumBands() const noexcept { return filterBank.getNumBands(); }

    /** Per-band gain, post GainEnvelope, for the most recently processed block's last sample (metering). */
    const std::vector<float>& getLastBandGains() const noexcept { return lastBandGains; }

private:
    void rebuildBandModules();

    juce::dsp::ProcessSpec spec {};
    presets::FrequencyProfile profile = presets::makeGuitarProfile();

    FilterBank filterBank;
    std::vector<EnvelopeFollower> envelopeFollowers;
    std::vector<NoiseFloorTracker> noiseTrackers;
    std::vector<TransientDetector> transientDetectors;
    std::vector<GateDecision> gateDecisions;
    std::vector<GainEnvelope> gainEnvelopes;

    std::vector<juce::AudioBuffer<float>> bandBuffers;
    std::vector<float> lastBandGains;

    float thresholdOffsetDb = 0.0f;
    float sensitivityMultiplier = 1.0f;
    float minGainOverride = -1.0f; // < 0 means "use profile default per band" -- engine still needs *a* Gmin per band, see .cpp
    float attackMul = 1.0f, holdMul = 1.0f, releaseMul = 1.0f;
    float hysteresisDb = 2.0f;
    float mix = 1.0f;
    bool bypassed = false;

    // Per-band scratch buffers reused every block (sized to
    // [numBands][spec.maximumBlockSize] in rebuildBandModules()), never
    // reallocated in process(). EnvelopeFollower/NoiseFloorTracker are
    // stateful (one-pole filters / history ring buffers), so each must be
    // fed exactly once per sample per block: pass 1 fills these once per
    // band, pass 2 re-reads them (for the coherence/bass-control macro
    // decision *and* the per-sample gate) instead of re-calling process().
    std::vector<std::vector<float>> bandEnvDb;
    std::vector<std::vector<float>> bandNoiseDb;
    std::vector<float> scratchShapedGain;

    // Also reused every block instead of being allocated on the audio thread.
    std::vector<float> bandSnrSnapshot;
    std::vector<float> bandEnergySnapshot;
    juce::AudioBuffer<float> dryBuffer;
};

} // namespace adaptivegate::dsp
