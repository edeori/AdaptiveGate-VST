#include "EnvelopeFollower.h"

#include <cmath>

namespace adaptivegate::dsp
{

namespace
{
    // Vanishingly small offset added before log10() so a fully silent signal
    // (currentSquared == 0) produces a large-but-finite negative dB value
    // instead of -inf, which is then clamped to minDb anyway.
    constexpr float tinyEpsilon = 1.0e-12f;

    /** One-pole coefficient for a given time constant, per the standard
        coeff = exp(-1 / (timeSeconds * sampleRate)) formula. A non-positive
        time constant is treated as "instantaneous" (coeff == 0), which makes
        the smoother jump straight to the new value on the very next sample. */
    float timeConstantToCoeff (float timeMs, double sampleRate)
    {
        if (timeMs <= 0.0f || sampleRate <= 0.0)
            return 0.0f;

        const double timeSeconds = (double) timeMs * 0.001;
        return (float) std::exp (-1.0 / (timeSeconds * sampleRate));
    }
}

void EnvelopeFollower::prepare (double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = newSampleRate;

    // Recompute coefficients against the new sample rate using whichever
    // time constants were last requested (defaults to instantaneous if
    // setTimeConstants() has never been called).
    reset();
}

void EnvelopeFollower::setTimeConstants (float attackMs, float releaseMs)
{
    attackCoeff = timeConstantToCoeff (attackMs, sampleRate);
    releaseCoeff = timeConstantToCoeff (releaseMs, sampleRate);
}

void EnvelopeFollower::process (const juce::AudioBuffer<float>& bandAudio, float* envelopeDbOut, int numSamples)
{
    jassert (envelopeDbOut != nullptr);
    jassert (numSamples <= bandAudio.getNumSamples());

    const int numCh = bandAudio.getNumChannels();
    const float* const* channels = bandAudio.getArrayOfReadPointers();

    for (int i = 0; i < numSamples; ++i)
    {
        // Collapse to mono via max-across-channels (not average) so the
        // envelope - and therefore the gate - stays sensitive to whichever
        // channel is loudest at this sample, rather than being diluted by a
        // quiet channel.
        float maxAbs = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            maxAbs = juce::jmax (maxAbs, std::abs (channels[ch][i]));

        const float squaredIn = maxAbs * maxAbs;

        // One-pole smoother on the squared (power) signal: rising energy uses
        // the attack coefficient, falling energy uses the release coefficient.
        const float coeff = (squaredIn > currentSquared) ? attackCoeff : releaseCoeff;
        currentSquared = squaredIn + coeff * (currentSquared - squaredIn);

        float db = 20.0f * std::log10 (std::sqrt (currentSquared) + tinyEpsilon);
        db = juce::jmax (db, minDb);

        envelopeDbOut[i] = db;
        currentDb = db;
    }
}

void EnvelopeFollower::reset()
{
    currentSquared = 0.0f;
    currentDb = minDb;
}

} // namespace adaptivegate::dsp
