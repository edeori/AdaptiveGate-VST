#pragma once

namespace adaptivegate::dsp
{

/**
    Detects fast rising energy (transients) from an envelope-in-dB stream so the
    gate can bias open quickly regardless of the current SNR estimate, per
    docs/description.md ("Drums require... transient detector").
*/
class TransientDetector
{
public:
    TransientDetector() = default;

    void prepare (double sampleRate, int maxBlockSize);

    /**
        thresholdDbPerMs: minimum rate of rise (dB/ms) in envelope to flag a transient.
        lookbackMs: window over which the rise is measured.
        decayMs: how long the transient "boost" output decays after being triggered.
    */
    void setSensitivity (float thresholdDbPerMs, float lookbackMs, float decayMs);

    /**
        Feed one envelope-in-dB sample. Returns true the sample a transient onset
        was detected. getTransientBoost() reports a decaying 0..1 value usable to
        bias the gate probability open (see GateDecision).
    */
    bool processSample (float envelopeDb);

    float getTransientBoost() const noexcept { return boost; }

    void reset();

private:
    double sampleRate = 44100.0;
    float thresholdDbPerMs = 6.0f;
    float lookbackMs = 5.0f;
    float decayMs = 50.0f;

    float previousDb = -100.0f;
    float boost = 0.0f;
    float boostDecayCoeff = 0.0f;

    // previousDb is deliberately updated as a slow-following one-pole reference
    // (time constant ~lookbackMs) rather than the immediately preceding sample, so
    // that envelopeDb - previousDb approximates the rise over the last lookbackMs
    // without needing a separate delay-line buffer. lookbackCoeff is that reference's
    // one-pole coefficient, derived from lookbackMs (see recomputeCoeffs()).
    float lookbackCoeff = 0.0f;

    void recomputeCoeffs();
};

} // namespace adaptivegate::dsp
