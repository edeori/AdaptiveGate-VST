#include "GateVisualizerComponent.h"
#include "UI/AdaptiveGateFonts.h"
#include "UI/AdaptiveGateLookAndFeel.h"

namespace
{
    constexpr float meterMinDb = -66.0f;
    constexpr float meterMaxDb = 0.0f;

    float dbToY (float db, juce::Rectangle<float> area)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (db - meterMinDb) / (meterMaxDb - meterMinDb));
        return area.getBottom() - t * area.getHeight();
    }

    void drawSpacedText (juce::Graphics& g, juce::String text, juce::Rectangle<int> area,
                         juce::Justification justification)
    {
        juce::String spaced;
        for (int i = 0; i < text.length(); ++i)
        {
            spaced << text[i];
            if (i + 1 < text.length()) spaced << " ";
        }
        g.drawFittedText (spaced, area, justification, 1);
    }
}

GateVisualizerComponent::GateVisualizerComponent (AdaptiveGateAudioProcessor& p)
    : processorRef (p)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

GateVisualizerComponent::~GateVisualizerComponent() { stopTimer(); }

juce::String GateVisualizerComponent::formatHz (float hz)
{
    if (hz >= 1000.0f)
        return juce::String (hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + "k";
    return juce::String ((int) hz);
}

void GateVisualizerComponent::timerCallback()
{
    const auto snapshot = processorRef.getMeterSnapshot();
    if (smoothedBands.size() != snapshot.size())
        smoothedBands.assign (snapshot.size(), SmoothedBand {});

    for (size_t i = 0; i < snapshot.size(); ++i)
    {
        auto& current = smoothedBands[i];
        const auto& target = snapshot[i];
        current.lowHz = target.lowHz;
        current.highHz = target.highHz;
        current.envelopeDb += (target.envelopeDb - current.envelopeDb) * 0.35f;
        current.noiseDb += (target.noiseDb - current.noiseDb) * 0.35f;
        current.thresholdDb += (target.thresholdDb - current.thresholdDb) * 0.35f;
        current.gain += (target.gain - current.gain) * 0.25f;
        current.isDriving = target.isDriving;
    }
    repaint();
}

void GateVisualizerComponent::paint (juce::Graphics& g)
{
    using L = adaptivegate::ui::LookAndFeel;
    const auto bounds = getLocalBounds().toFloat();
    const float scale = bounds.getWidth() / 778.0f;

    g.setColour (L::text);
    g.setFont (adaptivegate::ui::Fonts::title (31.0f * scale));
    g.drawText ("ADAPTIVE GATE", juce::Rectangle<int> (0, juce::roundToInt (21.0f * scale), getWidth(), juce::roundToInt (38.0f * scale)),
                juce::Justification::centred);
    g.setColour (L::textDim);
    g.setFont (adaptivegate::ui::Fonts::medium (9.0f * scale));
    drawSpacedText (g, "FREQUENCY-AWARE SIGNAL CONTROL",
                    juce::Rectangle<int> (0, juce::roundToInt (61.0f * scale), getWidth(), juce::roundToInt (16.0f * scale)),
                    juce::Justification::centred);

    g.setFont (adaptivegate::ui::Fonts::medium (7.5f * scale));
    const int legendY = juce::roundToInt (25.0f * scale);
    auto legend = [&] (const char* name, juce::Colour colour, int x)
    {
        g.setColour (colour); g.fillRect (juce::roundToInt ((float) x * scale), legendY + juce::roundToInt (5.0f * scale),
                                           juce::roundToInt (9.0f * scale), juce::jmax (1, juce::roundToInt (2.0f * scale)));
        g.setColour (L::textDim); g.drawText (name, juce::roundToInt (((float) x + 13.0f) * scale), legendY,
                                              juce::roundToInt (55.0f * scale), juce::roundToInt (13.0f * scale),
                                              juce::Justification::centredLeft);
    };
    legend ("ENV", L::cyanHot, 566);
    legend ("NOISE", juce::Colour (0xffaab3c0), 628);
    legend ("THRESHOLD", L::threshold, 699);
    auto graph = juce::Rectangle<float> (22.0f * scale, 104.0f * scale, 734.0f * scale, 250.0f * scale);
    for (int db = -60; db <= 0; db += 12)
    {
        g.setColour (juce::Colour (0xff939fb1).withAlpha (0.07f));
        g.drawHorizontalLine (juce::roundToInt (dbToY ((float) db, graph)), graph.getX(), graph.getRight());
    }

    if (smoothedBands.empty())
    {
        g.setColour (L::textDim.withAlpha (0.4f));
        g.setFont (adaptivegate::ui::Fonts::numeric (12.0f * scale));
        g.drawText ("WAITING FOR AUDIO", graph.toNearestInt(), juce::Justification::centred);
        return;
    }

    const float bandWidth = graph.getWidth() / (float) smoothedBands.size();
    int driver = 0;
    for (size_t i = 0; i < smoothedBands.size(); ++i)
    {
        const auto& band = smoothedBands[i];
        if (band.isDriving) driver = (int) i;
        auto column = juce::Rectangle<float> (graph.getX() + (float) i * bandWidth + bandWidth * 0.18f,
                                               graph.getY(), bandWidth * 0.64f, graph.getHeight());
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        g.fillRect (column);

        auto envelope = column.withTop (dbToY (band.envelopeDb, column));
        juce::ColourGradient fill (band.isDriving ? L::cyanHot.withAlpha (0.76f) : L::cyan.withAlpha (0.42f),
                                   envelope.getCentreX(), envelope.getY(), L::cyan.withAlpha (0.04f),
                                   envelope.getCentreX(), envelope.getBottom(), false);
        g.setGradientFill (fill); g.fillRect (envelope);

        g.setColour (juce::Colour (0xffb4becc).withAlpha (0.74f));
        g.drawHorizontalLine (juce::roundToInt (dbToY (band.noiseDb, column)), column.getX(), column.getRight());
        g.setColour (L::threshold.withAlpha (0.92f));
        g.fillRect (column.getX(), dbToY (band.thresholdDb, column) - 0.75f, column.getWidth(), 1.5f * scale);
        g.setColour (band.isDriving ? L::text.withAlpha (0.9f) : L::text.withAlpha (0.12f));
        g.drawRect (column, band.isDriving ? 2.0f * scale : 1.0f * scale);

        auto label = juce::Rectangle<int> (juce::roundToInt (graph.getX() + (float) i * bandWidth),
                                            juce::roundToInt (367.0f * scale), juce::roundToInt (bandWidth),
                                            juce::roundToInt (14.0f * scale));
        g.setColour (L::textDim);
        g.setFont (adaptivegate::ui::Fonts::numeric (8.0f * scale));
        g.drawFittedText (formatHz (band.lowHz) + "-" + formatHz (band.highHz), label, juce::Justification::centred, 1);
    }

    const float gain = juce::jlimit (0.0f, 1.0f, smoothedBands.front().gain);
    const auto centre = juce::Point<float> (bounds.getCentreX(), 238.0f * scale);
    auto readout = juce::Rectangle<float> (86.0f * scale, 58.0f * scale).withCentre (centre);
    g.setColour (juce::Colour (0xff030508).withAlpha (0.92f)); g.fillRoundedRectangle (readout, 5.0f * scale);
    g.setColour (L::text.withAlpha (0.2f)); g.drawRoundedRectangle (readout, 5.0f * scale, 1.0f);
    g.setColour (L::text); g.setFont (adaptivegate::ui::Fonts::numeric (24.0f * scale));
    g.drawText (juce::String (juce::roundToInt (gain * 100.0f)) + "%", readout.removeFromTop (37.0f * scale).toNearestInt(), juce::Justification::centredBottom);
    g.setColour (L::cyanHot); g.setFont (adaptivegate::ui::Fonts::medium (7.5f * scale));
    g.drawText (gain > 0.7f ? "GATE OPEN" : (gain > 0.2f ? "ADAPTING" : "GATE CLOSED"), readout.toNearestInt(), juce::Justification::centredTop);

    g.setColour (L::textDim); g.setFont (adaptivegate::ui::Fonts::medium (9.0f * scale));
    g.drawText ("DRIVING BAND", juce::roundToInt (23.0f * scale), juce::roundToInt (397.0f * scale),
                juce::roundToInt (90.0f * scale), juce::roundToInt (16.0f * scale), juce::Justification::centredLeft);
    g.setColour (L::cyanHot);
    g.drawText (formatHz (smoothedBands[(size_t) driver].lowHz) + "-" + formatHz (smoothedBands[(size_t) driver].highHz),
                juce::roundToInt (112.0f * scale), juce::roundToInt (397.0f * scale),
                juce::roundToInt (120.0f * scale), juce::roundToInt (16.0f * scale), juce::Justification::centredLeft);
}
