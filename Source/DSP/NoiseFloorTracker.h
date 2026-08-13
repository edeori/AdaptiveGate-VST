#pragma once

#include <vector>

namespace adaptivegate::dsp
{

/**
    Per-band noise floor estimator N(t), in dB, combining two signals:

    1. An ASYMMETRIC tracker: when the incoming envelope is falling, the noise
       estimate follows it quickly (fastFallMs); when the envelope is rising,
       the noise estimate follows very slowly (slowRiseMs), so transients are
       not mistaken for a rising noise floor.

    2. A MINIMUM-STATISTICS refinement: a rolling buffer of the last
       `windowSeconds` of envelope-in-dB values, from which a low percentile
       (e.g. the 20th percentile, `percentile` in [0,1]) is extracted as a
       secondary noise estimate.

    Final estimate = blendWeight * asymmetricEstimate + (1 - blendWeight) * percentileEstimate.
    Default blendWeight = 0.5. This exact blend formula is the fixed contract
    other modules (GateDecision via AdaptiveGateEngine) rely on.
*/
class NoiseFloorTracker
{
public:
    NoiseFloorTracker() = default;

    void prepare (double sampleRate, int maxBlockSize);

    /** Asymmetric one-pole follower time constants, in milliseconds. */
    void setAsymmetricTimes (float fastFallMs, float slowRiseMs);

    /** Minimum-statistics window length in seconds and percentile in [0,1] (e.g. 0.2 = 20th percentile). */
    void setMinStatsWindow (float windowSeconds, float percentile);

    /** Weight given to the asymmetric estimate vs. the percentile estimate, in [0,1]. */
    void setBlendWeight (float weight);

    /** Feed one envelope-in-dB sample, returns the updated noise floor estimate in dB. */
    float processSample (float envelopeDb);

    /** Convenience block API mirroring EnvelopeFollower::process. */
    void process (const float* envelopeDbIn, float* noiseDbOut, int numSamples);

    float getCurrentNoiseDb() const noexcept { return currentNoiseDb; }

    void reset();

private:
    double sampleRate = 44100.0;
    float fastFallCoeff = 0.0f;
    float slowRiseCoeff = 0.0f;
    float blendWeight = 0.5f;
    float percentile = 0.2f;
    float windowSeconds = 3.0f;

    // Raw time constants (ms) kept alongside the derived coefficients so prepare()
    // can safely recompute fastFallCoeff/slowRiseCoeff if the sample rate changes
    // (e.g. re-prepare without an explicit setAsymmetricTimes() call).
    float fastFallMs = 50.0f;
    float slowRiseMs = 4000.0f;

    float asymmetricEstimateDb = -60.0f;
    float currentNoiseDb = -60.0f;

    // Implementer: ring buffer (or histogram) of recent envelope-in-dB samples
    // sized for windowSeconds at the prepared sample rate, used for percentile lookup.
    std::vector<float> history;
    int writeIndex = 0;
    int filledCount = 0;

    // History is decimated (only every decimationFactor-th sample is stored) to keep
    // the buffer small regardless of sample rate, and the percentile is recomputed
    // (cached) once per newly stored history sample rather than every processSample().
    int decimationFactor = 1;
    int decimationCounter = 0;
    float percentileEstimateDb = -60.0f;

    void recomputeAsymmetricCoeffs();
    void resizeHistory();
    void updatePercentileEstimate();
};

} // namespace adaptivegate::dsp
