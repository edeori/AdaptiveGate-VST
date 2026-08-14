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

        Input -> FilterBank -> {EnvelopeFollower, NoiseFloorTracker, TransientDetector} (per band)
               -> per-band SNR -> margin adjustments (weighting/coherence/bass-control)
               -> per-sample: whichever band shows the strongest evidence of real signal
                  ("excess" = SNR - margin, in dB) drives ONE global GateDecision
               -> ONE global GainEnvelope (attack/hold/release borrowed from the driving band)
               -> that single gain is applied to the ORIGINAL (unsplit) signal -> Output (dry/wet mix)

    Detection is per-band (different frequency regions need different noise/SNR
    estimates), but the actual gate action is deliberately global/broadband: gating
    each band independently would open/close different frequency regions at
    different times, which sounds like the spectrum is being carved up rather than
    like a single coherent gate. FilterBank's split bands are only ever used to feed
    the per-band analysis modules below - they are never re-summed into the output.

    This is the integration point all the individual DSP modules are implemented
    against; it owns one instance of each per-band analysis module, one FilterBank,
    and exactly one GateDecision/GainEnvelope pair for the global gate.
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
    void setSensitivity (float sigmoidKMultiplier); // multiplies the global gate's sigmoidK
    void setMinGain (float gMin01);                 // overrides the global gate's Gmin (0..1)
    void setAttackHoldReleaseMultiplier (float attackMul, float holdMul, float releaseMul);
    void setHysteresisDb (float hysteresisDb);
    void setMix (float dryWet01);                    // 0 = dry, 1 = fully gated
    void setBypassed (bool shouldBypass);

    /** Processes in place; buffer channel count/size must match what prepare() was called with (or less). */
    void process (juce::AudioBuffer<float>& buffer);

    void reset();

    int getNumBands() const noexcept { return filterBank.getNumBands(); }

    /** Per-band detection state, plus the single global gate's gain, for GUI visualization. */
    struct BandMeter
    {
        float lowHz = 0.0f, highHz = 0.0f;
        float envelopeDb = -100.0f;
        float noiseDb = -100.0f;
        float thresholdDb = -100.0f; // noiseDb + resolved margin: the level envelopeDb must clear for this band to argue "open"
        float gain = 0.0f;           // the single global gate's gain (same value in every band's entry), [0,1]
        bool isDriving = false;      // true for whichever band currently has the strongest excess (drives the global decision)
    };

    /** Thread-safe: audio thread writes at the end of process(), any other thread may call this to read. */
    std::vector<BandMeter> getMeterSnapshot() const
    {
        const juce::SpinLock::ScopedLockType sl (meterLock);
        return meterSnapshot;
    }

private:
    void rebuildBandModules();
    void updateGlobalGateParameters();

    juce::dsp::ProcessSpec spec {};
    presets::FrequencyProfile profile = presets::makeGuitarProfile();

    FilterBank filterBank;
    std::vector<EnvelopeFollower> envelopeFollowers;
    std::vector<NoiseFloorTracker> noiseTrackers;
    std::vector<TransientDetector> transientDetectors;

    // Exactly one gate for the whole signal - see class comment for why.
    GateDecision globalGateDecision;
    GainEnvelope globalGainEnvelope;
    float globalBaseSigmoidK = 0.5f; // average of profile.bands[].sigmoidK, set in rebuildBandModules()
    int lastDrivingBandIndex = 0;    // last band whose attack/hold/release the global envelope is using

    std::vector<juce::AudioBuffer<float>> bandBuffers;

    float thresholdOffsetDb = 0.0f;
    float sensitivityMultiplier = 1.0f;
    float minGainOverride = -1.0f; // < 0 means "use 0 as Gmin" -- see updateGlobalGateParameters()
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
    std::vector<float> bandMarginDb;      // resolved per-band margin for the current block, computed once before the per-sample loop
    std::vector<float> scratchGlobalGain; // the single gate's gain, one value per sample in the block

    // Also reused every block instead of being allocated on the audio thread.
    std::vector<float> bandSnrSnapshot;
    std::vector<float> bandEnergySnapshot;
    juce::AudioBuffer<float> dryBuffer;

    // GUI metering: meterScratch is audio-thread-only (filled in process(), resized
    // in rebuildBandModules()); meterSnapshot is the copy any other thread reads via
    // getMeterSnapshot(), guarded by meterLock. The lock is only ever held for a
    // short vector copy, never across the whole block, so it can't cause audio dropouts.
    mutable juce::SpinLock meterLock;
    std::vector<BandMeter> meterSnapshot;
    std::vector<BandMeter> meterScratch;
};

} // namespace adaptivegate::dsp
