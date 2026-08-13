#pragma once

#include <vector>
#include <string>

namespace adaptivegate::presets
{

enum class SourceType
{
    Speech,
    Guitar,
    Bass,
    DrumCloseMic,
    DrumOverhead
};

/**
    Per-band adaptive behavior, one entry per FilterBank band, in ascending
    frequency order. Values are derived from the frequency-behavior tables and
    the source-weighting matrix in docs/description.md.
*/
struct BandProfile
{
    float lowHz = 0.0f;
    float highHz = 0.0f;

    /** W_k: source importance weight in [0,1]; 1.0 = strongly useful signal. */
    float weight = 1.0f;

    /** Base SNR margin M_base in dB for this band (before weighting adjustment). */
    float baseMarginDb = 4.0f;

    /** Gate shaping for this band. */
    float attackMs = 2.0f;
    float holdMs = 30.0f;
    float releaseMs = 80.0f;
    float sigmoidK = 0.5f;

    /**
        Final SNR margin, per docs/description.md: M_k = M_base + C * (1 - W_k).
        `coherenceMarginConstant` is the C used when calling this from the engine.
    */
    float resolveMarginDb (float coherenceMarginConstant) const
    {
        return baseMarginDb + coherenceMarginConstant * (1.0f - weight);
    }
};

/**
    A complete adaptive-gate configuration for one source type: band edges +
    per-band behavior, plus the two special-case behaviors called out in
    docs/description.md:

      - crossBandCoherenceGroups: indices of bands (into `bands`) that should be
        checked together; if several are simultaneously active, higher bands in
        `highBandsUnlockedByCoherence` get their margin relaxed.

      - bassControlledHighBand: for distorted bass, P_bass derived from low/mid
        band energy modulates the threshold of a high band (8-12kHz-ish).
*/
struct FrequencyProfile
{
    SourceType type = SourceType::Guitar;
    std::string name;

    std::vector<BandProfile> bands;

    /** M_base -> M_k weighting constant C, see BandProfile::resolveMarginDb. */
    float marginWeightConstant = 6.0f;

    // --- Cross-band coherence (guitar/bass) ---
    bool coherenceEnabled = false;
    std::vector<int> coherenceLowMidBandIndices;   // e.g. 100-400Hz, 400Hz-1.5kHz, 1.5-4kHz bands
    std::vector<int> coherenceHighBandIndices;      // e.g. 4-8kHz, 8-16kHz bands unlocked by coherence
    float coherenceMarginRelaxDb = 6.0f;             // how much to relax margin on high bands when coherent
    int coherenceMinActiveBands = 2;                 // how many low/mid bands must be active

    // --- Bass low-band-controls-high-band gate (distorted bass preset) ---
    bool bassControlEnabled = false;
    std::vector<int> bassControlSourceBandIndices;   // 60-400Hz + 400Hz-1.5kHz bands
    std::vector<int> bassControlTargetBandIndices;   // 8-12kHz band(s)
    float bassPresentMarginDb = 4.0f;                 // threshold decrease when bass present
    float bassAbsentMarginDb = 14.0f;                 // threshold increase when bass absent

    std::vector<float> getCrossoverFrequencies() const;
};

FrequencyProfile makeSpeechProfile();
FrequencyProfile makeGuitarProfile();
FrequencyProfile makeDistortedBassProfile();
FrequencyProfile makeDrumCloseMicProfile();
FrequencyProfile makeDrumOverheadProfile();

FrequencyProfile getProfileFor (SourceType type);

} // namespace adaptivegate::presets
