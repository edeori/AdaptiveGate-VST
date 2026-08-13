#pragma once

#include <vector>
#include "FrequencyProfile.h"

namespace adaptivegate::presets
{

/**
    Implements the two multi-band interaction rules from docs/description.md:

    1. Cross-band coherence: "If several [low/mid] bands are active together,
       there is a high probability that a real note is being played" -> relax
       the margin (make gate more permissive) on the associated high bands.

    2. Low-band controlled high-band gate (distorted bass): P_bass estimated
       from low/mid band energy modulates the threshold of a high band.

    Both functions take per-band SNR-in-dB (or energy-in-dB) snapshots for the
    *current* block and the profile, and return a per-band ADDITIVE ADJUSTMENT
    (in dB) to be applied to each band's resolved margin before calling
    GateDecision::computeGain (positive = stricter, negative = more permissive).
    Bands not affected by either rule get 0.0 adjustment.
*/
std::vector<float> computeCoherenceMarginAdjustments (const FrequencyProfile& profile,
                                                        const std::vector<float>& bandSnrDb);

std::vector<float> computeBassControlMarginAdjustments (const FrequencyProfile& profile,
                                                           const std::vector<float>& bandEnergyDb);

/** Convenience: sums both adjustments (zeros where a rule is disabled on the profile). */
std::vector<float> computeAllMarginAdjustments (const FrequencyProfile& profile,
                                                  const std::vector<float>& bandSnrDb,
                                                  const std::vector<float>& bandEnergyDb);

} // namespace adaptivegate::presets
