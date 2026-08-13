#include "GateDecision.h"

#include <cmath>
#include <algorithm>

namespace adaptivegate::dsp
{

void GateDecision::prepare (double /*sampleRate*/, int /*maxBlockSize*/)
{
    // GateDecision is a per-sample/per-block stateless-in-time-constant model
    // (no filter coefficients depend on sampleRate/blockSize); prepare() only
    // exists to satisfy the uniform module lifecycle used by AdaptiveGateEngine.
    reset();
}

void GateDecision::setParameters (float sigmoidSteepnessK, float minGainIn)
{
    k = sigmoidSteepnessK;
    minGain = std::clamp (minGainIn, 0.0f, 1.0f);
}

void GateDecision::setHysteresis (float hysteresisDbIn)
{
    hysteresisDb = std::max (0.0f, hysteresisDbIn);
}

float GateDecision::computeGain (float snrDb, float thresholdDb, float transientBoost)
{
    // Schmitt-trigger style hysteresis: the effective comparison threshold
    // depends on the *current* open/closed state, so a slowly-varying SNR
    // sitting near `thresholdDb` cannot cause rapid open/close chatter -
    // it has to cross a full hysteresisDb band to flip state.
    //
    //   closed -> needs snrDb to rise above thresholdDb + hysteresisDb/2 to open
    //   open   -> needs snrDb to fall below thresholdDb - hysteresisDb/2 to close
    //
    // Between those two rails (the hysteresis band) the state simply holds,
    // which is exactly what prevents chatter.
    const float halfHysteresis = hysteresisDb * 0.5f;
    const float openThreshold = thresholdDb + halfHysteresis;   // must clear this to open (from closed)
    const float closeThreshold = thresholdDb - halfHysteresis;  // must fall below this to close (from open)

    if (isOpen)
    {
        if (snrDb < closeThreshold)
            isOpen = false;
    }
    else
    {
        if (snrDb > openThreshold)
            isOpen = true;
    }

    // The effective threshold used for the sigmoid probability itself also
    // follows the current state (same rails as above), so the resulting
    // probability curve is consistent with the discrete state: once open,
    // the gate stays "easier to keep open" until SNR drops far enough;
    // once closed, it stays "harder to reopen" until SNR rises far enough.
    const float effectiveThreshold = isOpen ? closeThreshold : openThreshold;

    const float x = k * (snrDb - effectiveThreshold);
    float p = 1.0f / (1.0f + std::exp (-x));

    // Transients bias the gate open regardless of SNR/hysteresis state.
    p = std::max (p, transientBoost);

    lastProbability = p;

    return minGain + (1.0f - minGain) * p;
}

void GateDecision::reset()
{
    isOpen = false;
    lastProbability = 0.0f;
}

} // namespace adaptivegate::dsp
