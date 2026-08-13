#include "NoiseFloorTracker.h"

#include <algorithm>
#include <cmath>

namespace adaptivegate::dsp
{

namespace
{
    // Standard one-pole smoothing coefficient for a given time constant in
    // milliseconds: coeff = exp(-1 / (timeMs * 0.001 * sampleRate)).
    // Larger timeMs -> coeff closer to 1 -> slower response.
    float onePoleCoeff (float timeMs, double sampleRate)
    {
        if (timeMs <= 0.0f || sampleRate <= 0.0)
            return 0.0f;

        return std::exp (-1.0f / (timeMs * 0.001f * (float) sampleRate));
    }

    // How many decimated history samples per second to keep for the
    // minimum-statistics percentile. 200 Hz (5 ms resolution) is more than
    // enough to characterise a noise floor over a multi-second window while
    // keeping the buffer small (and nth_element() cheap) regardless of the
    // actual audio sample rate.
    constexpr double kHistoryRateHz = 200.0;
    constexpr int kMinHistoryCapacity = 8;
}

void NoiseFloorTracker::prepare (double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = newSampleRate;
    recomputeAsymmetricCoeffs();
    resizeHistory();
}

void NoiseFloorTracker::setAsymmetricTimes (float newFastFallMs, float newSlowRiseMs)
{
    fastFallMs = newFastFallMs;
    slowRiseMs = newSlowRiseMs;
    recomputeAsymmetricCoeffs();
}

void NoiseFloorTracker::setMinStatsWindow (float newWindowSeconds, float newPercentile)
{
    windowSeconds = newWindowSeconds;
    percentile = std::clamp (newPercentile, 0.0f, 1.0f);
    resizeHistory();
}

void NoiseFloorTracker::setBlendWeight (float weight)
{
    blendWeight = std::clamp (weight, 0.0f, 1.0f);
}

void NoiseFloorTracker::recomputeAsymmetricCoeffs()
{
    fastFallCoeff = onePoleCoeff (fastFallMs, sampleRate);
    slowRiseCoeff = onePoleCoeff (slowRiseMs, sampleRate);
}

void NoiseFloorTracker::resizeHistory()
{
    decimationFactor = sampleRate > 0.0
                            ? std::max (1, (int) std::round (sampleRate / kHistoryRateHz))
                            : 1;

    const int capacity = std::max (kMinHistoryCapacity,
                                    (int) std::round ((double) windowSeconds * kHistoryRateHz));

    history.assign ((size_t) capacity, currentNoiseDb);
    writeIndex = 0;
    filledCount = 0;
    decimationCounter = 0;
    percentileEstimateDb = currentNoiseDb;
}

void NoiseFloorTracker::updatePercentileEstimate()
{
    if (filledCount <= 0 || history.empty())
        return;

    // Valid entries always occupy [0, filledCount) until the ring buffer first
    // wraps (filledCount reaches capacity); order doesn't matter for a percentile,
    // so no unwrapping is required.
    std::vector<float> snapshot (history.begin(), history.begin() + filledCount);

    const int lastIdx = (int) snapshot.size() - 1;
    const int rankIdx = std::clamp ((int) std::round (percentile * (float) lastIdx), 0, lastIdx);

    std::nth_element (snapshot.begin(), snapshot.begin() + rankIdx, snapshot.end());
    percentileEstimateDb = snapshot[(size_t) rankIdx];
}

float NoiseFloorTracker::processSample (float envelopeDb)
{
    // 1) Asymmetric one-pole tracker: fast on the way down, slow on the way up.
    const float coeff = (envelopeDb < asymmetricEstimateDb) ? fastFallCoeff : slowRiseCoeff;
    asymmetricEstimateDb = coeff * asymmetricEstimateDb + (1.0f - coeff) * envelopeDb;

    // 2) Minimum-statistics: feed a decimated history buffer and refresh the
    // cached percentile estimate whenever a new decimated sample lands.
    if (! history.empty())
    {
        if (++decimationCounter >= decimationFactor)
        {
            decimationCounter = 0;

            history[(size_t) writeIndex] = envelopeDb;
            writeIndex = (writeIndex + 1) % (int) history.size();
            if (filledCount < (int) history.size())
                ++filledCount;

            updatePercentileEstimate();
        }
    }
    else
    {
        percentileEstimateDb = envelopeDb;
    }

    // Fixed blend contract relied on by downstream modules.
    currentNoiseDb = blendWeight * asymmetricEstimateDb + (1.0f - blendWeight) * percentileEstimateDb;
    return currentNoiseDb;
}

void NoiseFloorTracker::process (const float* envelopeDbIn, float* noiseDbOut, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        noiseDbOut[i] = processSample (envelopeDbIn[i]);
}

void NoiseFloorTracker::reset()
{
    asymmetricEstimateDb = -60.0f;
    currentNoiseDb = -60.0f;
    percentileEstimateDb = -60.0f;

    std::fill (history.begin(), history.end(), -60.0f);
    writeIndex = 0;
    filledCount = 0;
    decimationCounter = 0;
}

} // namespace adaptivegate::dsp
