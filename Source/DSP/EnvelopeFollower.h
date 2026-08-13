#pragma once

#include <juce_dsp/juce_dsp.h>

namespace adaptivegate::dsp
{

/**
    Fast RMS envelope follower for a single band, collapsed to mono (max across
    channels) and reported in dBFS. Used per-band as the "Fast RMS" block in the
    architecture diagram (docs/description.md).
*/
class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;

    void prepare (double sampleRate, int maxBlockSize);

    /** RMS window / smoothing time constants in milliseconds. */
    void setTimeConstants (float attackMs, float releaseMs);

    /**
        Processes one block of a single band's audio (any channel count) and
        writes one envelope-in-dB value per sample into envelopeDbOut
        (envelopeDbOut must have numSamples capacity).
    */
    void process (const juce::AudioBuffer<float>& bandAudio, float* envelopeDbOut, int numSamples);

    /** Current envelope value in dBFS (post most recent process() call). */
    float getCurrentDb() const noexcept { return currentDb; }

    void reset();

private:
    double sampleRate = 44100.0;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float currentSquared = 0.0f;
    float currentDb = -100.0f;

    static constexpr float minDb = -100.0f;
};

} // namespace adaptivegate::dsp
