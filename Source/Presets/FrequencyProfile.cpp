#include "FrequencyProfile.h"

#include <cstddef>

// All band data below is transcribed from docs/description.md:
//   - "Suggested Frequency Behavior" tables (guitar / distorted bass / drums)
//   - "Source Weighting Example" matrix (Speech / Guitar / Bass / Drum columns)
//   - "Cross-Band Coherence" section
//   - "Low-Band Controlled High-Band Gate" section
//
// Where the qualitative behavior table and the numeric weighting matrix use
// slightly different band edges, judgement calls were made to merge them into
// a single consistent, gapless band ladder per profile (documented per
// profile below). All profiles are extended to 20 kHz on the top edge even
// where the source table stops at 16 kHz, per the "covering ~20Hz-20kHz"
// requirement.

namespace adaptivegate::presets
{

namespace
{
    BandProfile makeBand (float lowHz, float highHz, float weight, float baseMarginDb,
                           float attackMs, float holdMs, float releaseMs, float sigmoidK)
    {
        BandProfile b;
        b.lowHz = lowHz;
        b.highHz = highHz;
        b.weight = weight;
        b.baseMarginDb = baseMarginDb;
        b.attackMs = attackMs;
        b.holdMs = holdMs;
        b.releaseMs = releaseMs;
        b.sigmoidK = sigmoidK;
        return b;
    }
}

std::vector<float> FrequencyProfile::getCrossoverFrequencies() const
{
    std::vector<float> crossovers;
    if (bands.size() < 2)
        return crossovers;

    crossovers.reserve (bands.size() - 1);
    for (size_t i = 0; i + 1 < bands.size(); ++i)
        crossovers.push_back (bands[i].highHz);

    return crossovers;
}

// ---------------------------------------------------------------------------
// Speech
// ---------------------------------------------------------------------------
// Band edges taken directly from the Source Weighting Example matrix rows:
// 20-60, 60-120, 120-250, 250-500, 500-1.5k, 1.5-4k, 4-8k, 8-16k (top edge
// extended to 20k). Weights are the "Speech" column verbatim. Speech has no
// coherence / bass-control behavior in the doc, so those stay disabled.
// baseMarginDb is kept uniform (4dB) per the task instructions, letting
// `weight` (via M_k = M_base + C*(1-W_k) in resolveMarginDb) do the
// differentiation between "aggressive" low-weight bands and "conservative"
// high-weight bands. Attack/hold/release are moderate, per speech being
// mostly sustained/syllabic rather than percussive.
FrequencyProfile makeSpeechProfile()
{
    FrequencyProfile p;
    p.type = SourceType::Speech;
    p.name = "Speech";
    p.marginWeightConstant = 6.0f;

    p.bands = {
        makeBand (20.0f,   60.0f,   0.2f, 4.0f, 3.0f, 20.0f,  80.0f,  0.4f),
        makeBand (60.0f,   120.0f,  0.5f, 4.0f, 2.5f, 25.0f,  90.0f,  0.5f),
        makeBand (120.0f,  250.0f,  0.8f, 4.0f, 2.0f, 30.0f,  100.0f, 0.6f),
        makeBand (250.0f,  500.0f,  1.0f, 4.0f, 1.5f, 35.0f,  110.0f, 0.7f),
        makeBand (500.0f,  1500.0f, 1.0f, 4.0f, 1.0f, 40.0f,  120.0f, 0.7f),
        makeBand (1500.0f, 4000.0f, 1.0f, 4.0f, 1.0f, 45.0f,  130.0f, 0.7f),
        makeBand (4000.0f, 8000.0f, 0.8f, 4.0f, 1.5f, 30.0f,  100.0f, 0.6f),
        makeBand (8000.0f, 20000.0f, 0.5f, 4.0f, 2.0f, 20.0f, 80.0f,  0.5f),
    };

    p.coherenceEnabled = false;
    p.bassControlEnabled = false;

    return p;
}

// ---------------------------------------------------------------------------
// Guitar (distorted guitar)
// ---------------------------------------------------------------------------
// Judgement call: the qualitative "Suggested Frequency Behavior" table (edges
// at 20/70/120/250/1000/3500/6000/10000) and the weighting-matrix Guitar
// column (edges at 20/60/120/250/500/1500/4000/8000/16000) don't line up
// exactly. Rather than creating an 12+ band Frankenstein ladder, the matrix's
// 8-band ladder is used as the single source of truth for edges (it also
// matches the band groupings referenced by the Cross-Band Coherence section:
// "100-400Hz / 400Hz-1.5kHz / 1.5-4kHz" approximate our 120-250+250-500,
// 500-1500, and 1500-4000 bands), and the qualitative table's
// aggressive/conservative labels + the "Example opening margins" dB values
// are mapped onto those same 8 bands for baseMarginDb:
//   100Hz-4kHz   -> +4..6dB   (bands 2-5: 120-250, 250-500, 500-1.5k, 1.5-4k)
//   4-7kHz       -> +6..8dB   (band 6: 4-8k, lower half)
//   7-14kHz      -> +10..14dB (band 6 upper half / band 7: 8-16k+)
// Band 0 (20-60Hz, "very aggressive" rumble) and band 1 (60-120Hz, "medium")
// sit below the doc's example margin range, so they're set by judgement,
// consistent with "very aggressive" meaning a high (strict) margin.
FrequencyProfile makeGuitarProfile()
{
    FrequencyProfile p;
    p.type = SourceType::Guitar;
    p.name = "Guitar";
    p.marginWeightConstant = 6.0f;

    p.bands = {
        // lowHz    highHz    weight  baseMarginDb attackMs holdMs releaseMs sigmoidK
        makeBand (20.0f,   60.0f,   0.1f, 8.0f,  5.0f, 20.0f, 100.0f, 0.5f), // rumble, very aggressive
        makeBand (60.0f,   120.0f,  0.5f, 6.0f,  4.0f, 25.0f, 110.0f, 0.5f), // palm-mute low end, medium
        makeBand (120.0f,  250.0f,  0.9f, 5.0f,  3.0f, 30.0f, 120.0f, 0.6f), // weight/body, conservative
        makeBand (250.0f,  500.0f,  1.0f, 5.0f,  2.5f, 35.0f, 130.0f, 0.6f), // body/note info, conservative
        makeBand (500.0f,  1500.0f, 1.0f, 5.0f,  2.0f, 40.0f, 150.0f, 0.7f), // body/note info + attack onset
        makeBand (1500.0f, 4000.0f, 1.0f, 4.0f,  1.5f, 45.0f, 150.0f, 0.7f), // attack/presence/riff clarity, very conservative
        makeBand (4000.0f, 8000.0f, 0.7f, 7.0f,  2.0f, 25.0f, 100.0f, 0.6f), // pick/fizz/presence -> amp fizz/hiss, medium->aggressive
        makeBand (8000.0f, 20000.0f, 0.3f, 12.0f, 3.0f, 15.0f, 80.0f,  0.5f), // mostly noise in high-gain tones, very aggressive
    };

    // Cross-Band Coherence (guitar example from docs/description.md):
    // low/mid group approximates "100-400Hz + 400Hz-1.5kHz + 1.5-4kHz" using
    // bands 2-5 (120-250, 250-500, 500-1.5k, 1.5-4k); if >= 2 of those are
    // active, relax the margin on the high bands (4-8k, 8-16k+).
    p.coherenceEnabled = true;
    p.coherenceLowMidBandIndices = { 2, 3, 4, 5 };
    p.coherenceHighBandIndices = { 6, 7 };
    p.coherenceMarginRelaxDb = 6.0f;
    p.coherenceMinActiveBands = 2;

    // Bass-controlled high-band gate is bass-specific in the doc.
    p.bassControlEnabled = false;

    return p;
}

// ---------------------------------------------------------------------------
// Distorted Bass
// ---------------------------------------------------------------------------
// Judgement call: unlike guitar, the bass table's own edges (20/35/100/400/
// ~1.5k/4k/8k) and its "musical interpretation" restatement (30-100 / 100-400
// / 400-1.5k / 1.5-4k / 4-8k / 8k+) are mutually consistent and also line up
// cleanly with the Low-Band-Controlled-High-Band and Cross-Band-Coherence
// sections (which explicitly reference 60-400Hz, 400Hz-1.5kHz, 1.5-4kHz,
// 4-8kHz, 8-16kHz). So bass uses its own dedicated 7-band ladder rather than
// the weighting-matrix's 8-band ladder; matrix weights are mapped onto the
// nearest matching band by ear/judgement (documented inline).
FrequencyProfile makeDistortedBassProfile()
{
    FrequencyProfile p;
    p.type = SourceType::Bass;
    p.name = "Distorted Bass";
    p.marginWeightConstant = 6.0f;

    p.bands = {
        // lowHz  highHz   weight  baseMarginDb attackMs holdMs releaseMs sigmoidK
        makeBand (20.0f,   35.0f,   0.1f,  9.0f, 5.0f, 20.0f, 100.0f, 0.5f), // rumble/useless sub, very aggressive
        makeBand (35.0f,   100.0f,  1.0f,  3.0f, 4.0f, 30.0f, 150.0f, 0.6f), // fundamental/weight, very conservative
        makeBand (100.0f,  400.0f,  0.9f,  4.0f, 3.0f, 35.0f, 150.0f, 0.6f), // bass body, conservative
        makeBand (400.0f,  1500.0f, 0.8f,  5.0f, 2.5f, 40.0f, 150.0f, 0.6f), // note definition/grind, moderately conservative
        makeBand (1500.0f, 4000.0f, 0.6f,  6.0f, 2.0f, 30.0f, 120.0f, 0.6f), // distortion/attack, medium
        makeBand (4000.0f, 8000.0f, 0.4f,  8.0f, 2.0f, 20.0f, 100.0f, 0.6f), // clank/fizz, stronger gating
        makeBand (8000.0f, 20000.0f, 0.15f, 10.0f, 3.0f, 15.0f, 80.0f, 0.5f), // hiss, very aggressive (baseline; see bassControl below)
    };

    // Cross-Band Coherence: "This is especially useful for distorted bass."
    // low/mid group = 100-400Hz, 400Hz-1.5kHz, 1.5-4kHz (bands 2,3,4);
    // unlocks 4-8kHz and 8-16kHz+ (bands 5,6).
    p.coherenceEnabled = true;
    p.coherenceLowMidBandIndices = { 2, 3, 4 };
    p.coherenceHighBandIndices = { 5, 6 };
    p.coherenceMarginRelaxDb = 6.0f;
    p.coherenceMinActiveBands = 2;

    // Low-Band Controlled High-Band Gate: P_bass estimated from 60-400Hz +
    // 400Hz-1.5kHz. Our ladder splits "60-400Hz" across the 35-100Hz
    // (fundamental) and 100-400Hz (body) bands, so both are included as the
    // closest approximation; band 3 (400-1500Hz) is the "400Hz-1.5kHz" term.
    // Target is the 8-16kHz+ hiss band (only high band available).
    p.bassControlEnabled = true;
    p.bassControlSourceBandIndices = { 1, 2, 3 };
    p.bassControlTargetBandIndices = { 6 };
    p.bassPresentMarginDb = 4.0f;
    p.bassAbsentMarginDb = 14.0f;

    return p;
}

// ---------------------------------------------------------------------------
// Drums - shared band ladder for close-mic and overhead variants
// ---------------------------------------------------------------------------
// The Drum Preset table has its own clean edges (20/50/120/250/800/2.5k/6k/
// 12k/20k) which are used directly rather than the weighting matrix's edges,
// since the "Recommended detector behavior" section (fast opening for
// 50-250Hz and 2-6kHz) refers to sub-ranges that only make sense against
// this ladder. Matrix Drum-column weights are mapped by judgement onto the
// nearest band.
namespace
{
    std::vector<BandProfile> makeDrumBaseBands()
    {
        return {
            // lowHz   highHz    weight  baseMarginDb attackMs holdMs releaseMs sigmoidK
            makeBand (20.0f,   50.0f,   0.3f, 8.0f, 5.0f, 15.0f, 60.0f,  0.5f),  // rumble
            makeBand (50.0f,   120.0f,  1.0f, 3.0f, 0.5f, 40.0f, 100.0f, 0.9f),  // kick fundamental, very fast opening
            makeBand (120.0f,  250.0f,  0.9f, 4.0f, 0.5f, 35.0f, 90.0f,  0.9f),  // tom/snare body, very fast opening (50-250Hz rule)
            makeBand (250.0f,  800.0f,  0.7f, 5.0f, 3.0f, 20.0f, 70.0f,  0.6f),  // shell tone/boxiness
            makeBand (800.0f,  2500.0f, 0.85f, 3.5f, 2.0f, 25.0f, 80.0f, 0.7f),  // attack/stick
            makeBand (2500.0f, 6000.0f, 1.0f, 3.0f, 0.5f, 20.0f, 70.0f,  0.9f),  // crack/presence, very fast opening (2-6kHz rule)
            makeBand (6000.0f, 12000.0f, 0.6f, 7.0f, 3.0f, 15.0f, 50.0f, 0.6f),  // cymbals (close-mic default: aggressive)
            makeBand (12000.0f, 20000.0f, 0.4f, 9.0f, 4.0f, 10.0f, 40.0f, 0.5f), // cymbal air/hiss (close-mic default: aggressive)
        };
    }
}

// Close-Mic Drums: "aggressive high-band gating can be acceptable" -> keep
// the >=6kHz bands from the shared base table as-is (aggressive weights/
// margins/short envelope times). No coherence or bass-control rule is
// described for drums in the doc, so both stay disabled.
FrequencyProfile makeDrumCloseMicProfile()
{
    FrequencyProfile p;
    p.type = SourceType::DrumCloseMic;
    p.name = "Drums (Close-Mic)";
    p.marginWeightConstant = 6.0f;

    p.bands = makeDrumBaseBands();

    p.coherenceEnabled = false;
    p.bassControlEnabled = false;

    return p;
}

// Drum Bus / Overheads: "Do not aggressively gate the high-frequency region
// because cymbals live there... much more conservative above 6 kHz." Raises
// weight and baseMarginDb for the 6-12kHz and 12-20kHz bands relative to the
// close-mic variant (higher weight -> smaller resolved margin via
// resolveMarginDb's C*(1-W) term -> gate opens more easily / less gating).
// Also lengthens hold/release on those two bands so cymbal decay tails
// aren't choked.
FrequencyProfile makeDrumOverheadProfile()
{
    FrequencyProfile p;
    p.type = SourceType::DrumOverhead;
    p.name = "Drums (Overhead)";
    p.marginWeightConstant = 6.0f;

    p.bands = makeDrumBaseBands();

    // Overwrite the >=6kHz bands to be much more conservative than close-mic.
    p.bands[6] = makeBand (6000.0f,  12000.0f, 0.95f, 3.0f, 3.0f, 60.0f, 200.0f, 0.5f);
    p.bands[7] = makeBand (12000.0f, 20000.0f, 0.85f, 4.0f, 4.0f, 80.0f, 250.0f, 0.5f);

    p.coherenceEnabled = false;
    p.bassControlEnabled = false;

    return p;
}

FrequencyProfile getProfileFor (SourceType type)
{
    switch (type)
    {
        case SourceType::Speech:       return makeSpeechProfile();
        case SourceType::Guitar:       return makeGuitarProfile();
        case SourceType::Bass:         return makeDistortedBassProfile();
        case SourceType::DrumCloseMic: return makeDrumCloseMicProfile();
        case SourceType::DrumOverhead: return makeDrumOverheadProfile();
    }

    return makeGuitarProfile();
}

} // namespace adaptivegate::presets
