#pragma once

namespace adaptivegate::dsp
{

/**
    Shapes a target gain stream (0..1, from GateDecision::computeGain) into a
    smooth, generated gain envelope using attack / hold / release timing, per
    docs/description.md ("Nem egy mute switch, hanem generált gain envelope").

    State machine per sample:
      - target > current  -> ramp toward target over attackMs
      - target held at/above a recent peak -> stay at peak for holdMs before allowing release
      - target < current (post-hold) -> ramp toward target over releaseMs
*/
class GainEnvelope
{
public:
    GainEnvelope() = default;

    void prepare (double sampleRate, int maxBlockSize);

    void setTimes (float attackMs, float holdMs, float releaseMs);

    /** Feed one target gain sample in [0,1], returns the shaped output gain in [0,1]. */
    float processSample (float targetGain);

    /** Block convenience API. */
    void process (const float* targetGainIn, float* gainOut, int numSamples);

    float getCurrentGain() const noexcept { return currentGain; }

    void reset();

private:
    double sampleRate = 44100.0;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    int holdSamples = 0;

    float currentGain = 0.0f;
    int holdCounter = 0;
};

} // namespace adaptivegate::dsp
