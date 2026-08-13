#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace adaptivegate::dsp
{

/**
    Splits a mono/stereo audio stream into N phase-coherent bands using cascaded
    Linkwitz-Riley crossovers (juce::dsp::LinkwitzRileyFilter). Bands sum back to
    a flat response when unmodified, which lets the engine re-sum per-band gated
    signals into a full-range output.

    Contract (do not change signatures used by AdaptiveGateEngine without updating
    the engine): prepare() -> setCrossoverFrequencies() -> process() per block.
*/
class FilterBank
{
public:
    FilterBank() = default;

    /** Allocates filter state and per-band scratch buffers. */
    void prepare (const juce::dsp::ProcessSpec& spec);

    /**
        Defines N-1 crossover points in Hz (ascending, within Nyquist), producing N bands.
        E.g. {70, 120, 250, 1000, 3500, 6000, 10000} -> 8 bands.
        Safe to call after prepare(); reallocates internal filter chain.
    */
    void setCrossoverFrequencies (const std::vector<float>& crossoverHz);

    int getNumBands() const noexcept { return (int) crossovers.size() + 1; }

    /** Returns the [lowHz, highHz) edges for band index, using 0 and Nyquist as outer bounds. */
    std::pair<float, float> getBandEdges (int bandIndex) const;

    /**
        Splits `input` into getNumBands() bands, writing each band's audio into
        bandBuffers[i] (same channel count/size as input). bandBuffers must already
        contain getNumBands() buffers sized via prepare(); this call resizes samples
        as needed but does not reallocate per-block.
    */
    void process (const juce::AudioBuffer<float>& input, std::vector<juce::AudioBuffer<float>>& bandBuffers);

    void reset();

private:
    std::vector<float> crossovers;
    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    int numChannels = 2;

    // Cascaded-tree Linkwitz-Riley chain: filters[k] splits the running "high"
    // signal from filters[k-1] (or the original input for k == 0) into a low
    // band (-> band k) and a high band that feeds filters[k + 1] (or becomes
    // the last band once all crossovers are consumed). Each filter instance
    // uses the TPT dual-output processSample() overload, which derives its
    // low/high outputs from shared state so low + high always reconstructs
    // the filter's input exactly (phase-coherent, flat-sum reconstruction).
    std::vector<juce::dsp::LinkwitzRileyFilter<float>> filters;

    // Scratch array of per-band write pointers for the current channel being
    // processed, reused every block (resized only when the filter chain is
    // rebuilt) so process() never allocates.
    std::vector<float*> bandWritePointers;

    /** Rebuilds `filters` and `bandWritePointers` for the current crossovers/spec. */
    void rebuildFilters();
};

} // namespace adaptivegate::dsp
