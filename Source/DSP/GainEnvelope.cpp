#include "GainEnvelope.h"

#include <cmath>
#include <algorithm>

namespace adaptivegate::dsp
{

void GainEnvelope::prepare (double sampleRateIn, int /*maxBlockSize*/)
{
    sampleRate = sampleRateIn;
    reset();
}

void GainEnvelope::setTimes (float attackMs, float holdMs, float releaseMs)
{
    // Standard one-pole time-constant coefficients: coeff = exp(-1 / (T_seconds * sampleRate)).
    // Guard against non-positive times (which would make the coefficient undefined/instant).
    const float attackSeconds = std::max (attackMs, 0.001f) * 0.001f;
    const float releaseSeconds = std::max (releaseMs, 0.001f) * 0.001f;

    attackCoeff = std::exp (-1.0f / (attackSeconds * (float) sampleRate));
    releaseCoeff = std::exp (-1.0f / (releaseSeconds * (float) sampleRate));

    holdSamples = (int) std::lround (std::max (0.0f, holdMs) * 0.001f * (float) sampleRate);
}

float GainEnvelope::processSample (float targetGain)
{
    if (targetGain > currentGain)
    {
        // Attacking: ramp up toward the (higher) target, and (re)trigger the
        // hold period - a new/rising peak always extends the hold window so
        // the gate doesn't start releasing mid-attack.
        currentGain = targetGain + (currentGain - targetGain) * attackCoeff;
        holdCounter = holdSamples;
    }
    else
    {
        // Target is at or below current: either we're holding a recent peak,
        // or we're free to release toward the (lower) target.
        if (holdCounter > 0)
        {
            --holdCounter;
            // currentGain intentionally left unchanged while holding.
        }
        else
        {
            currentGain = targetGain + (currentGain - targetGain) * releaseCoeff;
        }
    }

    return currentGain;
}

void GainEnvelope::process (const float* targetGainIn, float* gainOut, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        gainOut[i] = processSample (targetGainIn[i]);
}

void GainEnvelope::reset()
{
    currentGain = 0.0f;
    holdCounter = 0;
}

} // namespace adaptivegate::dsp
