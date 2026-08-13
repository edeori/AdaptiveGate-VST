#include "FilterBank.h"

namespace adaptivegate::dsp
{

void FilterBank::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    maxBlockSize = (int) spec.maximumBlockSize;
    numChannels = (int) spec.numChannels;

    rebuildFilters();
}

void FilterBank::setCrossoverFrequencies (const std::vector<float>& crossoverHz)
{
    crossovers = crossoverHz;

    rebuildFilters();
}

void FilterBank::rebuildFilters()
{
    // juce::dsp::LinkwitzRileyFilter is a 4th-order (24 dB/oct) TPT filter whose
    // dual-output processSample() overload produces a low-pass and a high-pass
    // output from one shared state such that low + high reconstructs the input
    // exactly. Chaining one such filter per crossover point - feeding each
    // filter's high output into the next filter's input - forms the standard
    // cascaded-tree crossover: band k is the low output of filters[k], and the
    // final band is whatever "high" remains after the last crossover.
    filters.clear();
    filters.resize (crossovers.size());

    juce::dsp::ProcessSpec spec { sampleRate,
                                   (juce::uint32) maxBlockSize,
                                   (juce::uint32) numChannels };

    for (auto& filter : filters)
        filter.prepare (spec);

    for (size_t k = 0; k < crossovers.size(); ++k)
        filters[k].setCutoffFrequency (crossovers[k]);

    // One write pointer slot per band, reused (never reallocated) in process().
    bandWritePointers.assign ((size_t) getNumBands(), nullptr);
}

std::pair<float, float> FilterBank::getBandEdges (int bandIndex) const
{
    const int numBands = getNumBands();
    jassert (bandIndex >= 0 && bandIndex < numBands);

    const float nyquist = (float) (sampleRate * 0.5);

    const float low  = (bandIndex == 0) ? 0.0f : crossovers[(size_t) (bandIndex - 1)];
    const float high = (bandIndex == numBands - 1) ? nyquist : crossovers[(size_t) bandIndex];

    return { low, high };
}

void FilterBank::process (const juce::AudioBuffer<float>& input, std::vector<juce::AudioBuffer<float>>& bandBuffers)
{
    const int numBands = getNumBands();
    jassert ((int) bandBuffers.size() >= numBands);
    jassert ((int) bandWritePointers.size() >= numBands);

    const int numSamples = input.getNumSamples();
    const int numCh = input.getNumChannels();
    const int numCrossovers = (int) crossovers.size();

    // Resize each band buffer's logical sample count to match the block, but
    // never reallocate its underlying storage: prepare()/setCrossoverFrequencies()
    // already sized these upstream (they own the buffers passed in by the
    // engine), so avoidReallocating == true keeps this call allocation-free.
    for (int b = 0; b < numBands; ++b)
        bandBuffers[(size_t) b].setSize (numCh, numSamples, false, false, true);

    const float* const* inputChannels = input.getArrayOfReadPointers();

    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* in = inputChannels[ch];

        for (int b = 0; b < numBands; ++b)
            bandWritePointers[(size_t) b] = bandBuffers[(size_t) b].getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float current = in[i];

            // Walk the crossover chain: each filter peels off a low band and
            // hands the residual high band to the next filter in the tree.
            for (int k = 0; k < numCrossovers; ++k)
            {
                float low = 0.0f, high = 0.0f;
                filters[(size_t) k].processSample (ch, current, low, high);

                bandWritePointers[(size_t) k][i] = low;
                current = high;
            }

            // Whatever remains after the last crossover is the top band
            // (or, if there are no crossovers at all, the untouched input).
            bandWritePointers[(size_t) numCrossovers][i] = current;
        }
    }
}

void FilterBank::reset()
{
    for (auto& filter : filters)
        filter.reset();
}

} // namespace adaptivegate::dsp
