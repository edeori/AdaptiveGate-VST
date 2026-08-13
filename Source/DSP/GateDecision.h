#pragma once

namespace adaptivegate::dsp
{

/**
    Per-band soft gate decision, implementing the probabilistic model from
    docs/description.md:

        SNR(t) = E(t) - N(t)                          [dB, computed by caller]
        p = sigmoid(k * (SNR - T))                     [T = threshold/margin in dB]
        G = Gmin + (1 - Gmin) * p

    This is NOT a binary decision tree: G is a continuous target gain in [Gmin, 1].
    Hysteresis is applied by using a different effective threshold depending on
    whether the gate is currently judged "open" or "closed" (Schmitt-trigger style),
    to avoid chattering ("A gate kattogása elkerülése érdekében hysteresist használunk").

    Frequency-dependent margins, source weighting, cross-band coherence, and the
    bass-controlled high-band threshold modulation are all expected to be applied
    by the caller (AdaptiveGateEngine) by adjusting `thresholdDb` per band per
    block before calling computeGain(); this class stays generic/reusable.
*/
class GateDecision
{
public:
    GateDecision() = default;

    void prepare (double sampleRate, int maxBlockSize);

    /** k = sigmoid steepness, Gmin = minimum gain floor in [0,1] (e.g. 0.0 = full gate, 0.1 = -20dB floor). */
    void setParameters (float sigmoidSteepnessK, float minGain);

    /** Extra dB the SNR must clear/fall below to flip state, once in the opposite state. */
    void setHysteresis (float hysteresisDb);

    /**
        snrDb: E(t) - N(t) for this band this sample/block.
        thresholdDb: T, already including preset margin / weighting / coherence / bass-control adjustments.
        transientBoost: 0..1 from TransientDetector; biases p upward (p = max(p, transientBoost)).
        Returns target gain G in [minGain, 1], BEFORE attack/hold/release shaping (see GainEnvelope).
    */
    float computeGain (float snrDb, float thresholdDb, float transientBoost = 0.0f);

    /** Current probability-of-signal estimate (post most recent computeGain call), for metering/debug. */
    float getLastProbability() const noexcept { return lastProbability; }

    void reset();

private:
    float k = 0.5f;
    float minGain = 0.0f;
    float hysteresisDb = 2.0f;
    bool isOpen = false;
    float lastProbability = 0.0f;
};

} // namespace adaptivegate::dsp
