#include "Coherence.h"

#include <cstddef>

namespace adaptivegate::presets
{

namespace
{
    // Presence threshold for the bass-control estimator, in dB. AdaptiveGateEngine
    // seeds its per-block energy/SNR snapshots at -60dB as a "silence" baseline
    // (see bandEnergySnapshot / bandSnrSnapshot init in AdaptiveGateEngine::process),
    // so -40dB represents a signal clearly above that floor (~20dB headroom) without
    // requiring near-full-scale energy - a reasonable, if approximate, "is there
    // actually a bass note here" cutoff. Documented here since the doc only gives
    // the conceptual P_bass = f(E_low, E_mid) relationship, not a concrete threshold.
    constexpr float kBassPresenceThresholdDb = -40.0f;
}

std::vector<float> computeCoherenceMarginAdjustments (const FrequencyProfile& profile,
                                                        const std::vector<float>& bandSnrDb)
{
    std::vector<float> adjustments (bandSnrDb.size(), 0.0f);

    if (! profile.coherenceEnabled)
        return adjustments;

    // Count how many of the low/mid "note-related" bands currently have
    // signal above their noise floor (SNR > 0dB), i.e. are "active".
    int activeCount = 0;
    for (int idx : profile.coherenceLowMidBandIndices)
    {
        if (idx >= 0 && (size_t) idx < bandSnrDb.size() && bandSnrDb[(size_t) idx] > 0.0f)
            ++activeCount;
    }

    // "If several of these bands are active together, there is a high
    // probability that a real note is being played. In this case, higher
    // bands can be allowed to open" -> relax (negative = more permissive)
    // the margin on the associated high bands.
    if (activeCount >= profile.coherenceMinActiveBands)
    {
        for (int idx : profile.coherenceHighBandIndices)
        {
            if (idx >= 0 && (size_t) idx < adjustments.size())
                adjustments[(size_t) idx] = -profile.coherenceMarginRelaxDb;
        }
    }

    return adjustments;
}

std::vector<float> computeBassControlMarginAdjustments (const FrequencyProfile& profile,
                                                          const std::vector<float>& bandEnergyDb)
{
    std::vector<float> adjustments (bandEnergyDb.size(), 0.0f);

    if (! profile.bassControlEnabled)
        return adjustments;

    // P_bass estimate: simple average energy (in dB) across the low/mid
    // source bands (60-400Hz + 400Hz-1.5kHz per the doc). Average is used
    // rather than max/min so a single noisy or dead band doesn't dominate
    // the decision - this is a deliberately simple stand-in for the doc's
    // unspecified "P_bass = f(E_60-400Hz, E_400-1500Hz)".
    int validCount = 0;
    float sumDb = 0.0f;
    for (int idx : profile.bassControlSourceBandIndices)
    {
        if (idx >= 0 && (size_t) idx < bandEnergyDb.size())
        {
            sumDb += bandEnergyDb[(size_t) idx];
            ++validCount;
        }
    }

    const bool bassPresent = validCount > 0 && (sumDb / (float) validCount) > kBassPresenceThresholdDb;

    // Semantics (matches the BandProfile field doc comments directly):
    //   bassPresentMarginDb ("threshold decrease when bass present") is applied
    //   as a NEGATIVE adjustment -> more permissive, lets high-band distortion
    //   harmonics through.
    //   bassAbsentMarginDb ("threshold increase when bass absent") is applied
    //   as a POSITIVE adjustment -> stricter, suppresses standalone hiss.
    // This is the simpler of the two additive readings offered in the header
    // doc comment: the adjustment is used directly as a signed delta on top
    // of the target band's baseMarginDb, rather than trying to replace the
    // resolved margin outright.
    const float delta = bassPresent ? -profile.bassPresentMarginDb : profile.bassAbsentMarginDb;

    for (int idx : profile.bassControlTargetBandIndices)
    {
        if (idx >= 0 && (size_t) idx < adjustments.size())
            adjustments[(size_t) idx] = delta;
    }

    return adjustments;
}

std::vector<float> computeAllMarginAdjustments (const FrequencyProfile& profile,
                                                 const std::vector<float>& bandSnrDb,
                                                 const std::vector<float>& bandEnergyDb)
{
    auto coherence = computeCoherenceMarginAdjustments (profile, bandSnrDb);
    auto bassControl = computeBassControlMarginAdjustments (profile, bandEnergyDb);

    const size_t n = coherence.size() > bassControl.size() ? coherence.size() : bassControl.size();
    std::vector<float> total (n, 0.0f);

    for (size_t i = 0; i < coherence.size(); ++i)
        total[i] += coherence[i];

    for (size_t i = 0; i < bassControl.size(); ++i)
        total[i] += bassControl[i];

    return total;
}

} // namespace adaptivegate::presets
