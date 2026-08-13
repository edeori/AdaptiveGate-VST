#include "TransientDetector.h"

#include <algorithm>
#include <cmath>

namespace adaptivegate::dsp
{

namespace
{
    // Standard one-pole smoothing coefficient for a given time constant in
    // milliseconds: coeff = exp(-1 / (timeMs * 0.001 * sampleRate)).
    float onePoleCoeff (float timeMs, double sampleRate)
    {
        if (timeMs <= 0.0f || sampleRate <= 0.0)
            return 0.0f;

        return std::exp (-1.0f / (timeMs * 0.001f * (float) sampleRate));
    }
}

void TransientDetector::prepare (double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = newSampleRate;
    recomputeCoeffs();
}

void TransientDetector::setSensitivity (float newThresholdDbPerMs, float newLookbackMs, float newDecayMs)
{
    thresholdDbPerMs = newThresholdDbPerMs;
    lookbackMs = newLookbackMs;
    decayMs = newDecayMs;
    recomputeCoeffs();
}

void TransientDetector::recomputeCoeffs()
{
    boostDecayCoeff = onePoleCoeff (decayMs, sampleRate);
    lookbackCoeff = onePoleCoeff (lookbackMs, sampleRate);
}

bool TransientDetector::processSample (float envelopeDb)
{
    // previousDb is a slow-following reference with a ~lookbackMs time constant
    // (see header comment), so envelopeDb - previousDb approximates the rise over
    // the last lookbackMs without needing a delay-line buffer. Dividing by the
    // window length converts that rise into a dB/ms rate comparable to
    // thresholdDbPerMs.
    const float lookbackWindowMs = std::max (lookbackMs, 0.001f);
    const float riseRateDbPerMs = (envelopeDb - previousDb) / lookbackWindowMs;

    bool onsetDetected = false;

    if (riseRateDbPerMs >= thresholdDbPerMs)
    {
        boost = 1.0f;
        onsetDetected = true;
    }
    else
    {
        boost *= boostDecayCoeff;
    }

    // Advance the lagged reference toward the current value.
    previousDb = lookbackCoeff * previousDb + (1.0f - lookbackCoeff) * envelopeDb;

    return onsetDetected;
}

void TransientDetector::reset()
{
    previousDb = -100.0f;
    boost = 0.0f;
}

} // namespace adaptivegate::dsp
