#include "PluginEditor.h"
#include "UI/AdaptiveGateFonts.h"
#include "BinaryData.h"

namespace
{
    constexpr float designWidth = 1100.0f;
    constexpr float designHeight = 720.0f;

    juce::String spaced (const juce::String& text)
    {
        juce::String result;
        for (int i = 0; i < text.length(); ++i)
        {
            result << text[i];
            if (i + 1 < text.length()) result << " ";
        }
        return result;
    }
}

AdaptiveGateAudioProcessorEditor::AdaptiveGateAudioProcessorEditor (AdaptiveGateAudioProcessor& p)
    : AudioProcessorEditor (&p), controlPanel (p), visualizer (p), presetPanel (p), pedalView (p)
{
    setLookAndFeel (&adaptiveLookAndFeel);

    backgroundImage = juce::ImageFileFormat::loadFrom (BinaryData::background_png, (size_t) BinaryData::background_pngSize);
    logoImage = juce::ImageFileFormat::loadFrom (BinaryData::moth_logo_png, (size_t) BinaryData::moth_logo_pngSize);

    addAndMakeVisible (controlPanel);
    addAndMakeVisible (visualizer);
    addAndMakeVisible (presetPanel);
    addChildComponent (pedalView); // hidden until switchToView(Pedal)

    for (auto* target : { &makerLayoutTarget, &logoLayoutTarget, &versionLayoutTarget })
    {
        target->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*target);
    }

    pedalViewButton.setTooltip ("Switch to the simplified pedal view.");
    pedalViewButton.getProperties().set (adaptivegate::ui::LookAndFeel::chromeProperty, true);
    pedalViewButton.onClick = [this] { switchToView (ViewMode::Pedal); };
    addAndMakeVisible (pedalViewButton);

    pedalView.onSwitchToAdvanced = [this] { switchToView (ViewMode::Native); };

#if defined (JUCE_LAYOUT_TUNER) && JUCE_LAYOUT_TUNER
    layoutTuner = std::make_unique<juce_layout_tuner::Overlay> (
        *this, juce::Rectangle<int> (0, 0, (int) designWidth, (int) designHeight), 8);
    layoutTuner->addTarget (visualizer, "visualizer");
    layoutTuner->addTarget (presetPanel, "presetPanel");
    layoutTuner->addTarget (makerLayoutTarget, "makerLayoutTarget");
    layoutTuner->addTarget (logoLayoutTarget, "logoLayoutTarget");
    layoutTuner->addTarget (versionLayoutTarget, "versionLayoutTarget");
    addAndMakeVisible (*layoutTuner);
    layoutTuner->activate (true);
#endif

    setResizable (true, true);
    switchToView (ViewMode::Pedal);
}

AdaptiveGateAudioProcessorEditor::~AdaptiveGateAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void AdaptiveGateAudioProcessorEditor::paint (juce::Graphics& g)
{
    using L = adaptivegate::ui::LookAndFeel;
    g.fillAll (L::background);

    if (currentView != ViewMode::Native)
        return; // PedalViewComponent paints its own background/artwork over the whole window

    if (backgroundImage.isValid())
        g.drawImage (backgroundImage, getLocalBounds().toFloat());

    const float scale = (float) getWidth() / designWidth;
    g.setColour (juce::Colour (0xffa8afbb));
    g.setFont (adaptivegate::ui::Fonts::light (12.0f * scale));
    g.drawFittedText (spaced ("MOTH PRODUCTION"), makerLayoutTarget.getBounds(), juce::Justification::centredLeft, 1);

    if (logoImage.isValid())
        g.drawImageWithin (logoImage, logoLayoutTarget.getX(), logoLayoutTarget.getY(),
                           logoLayoutTarget.getWidth(), logoLayoutTarget.getHeight(), juce::RectanglePlacement::centred);

    g.setColour (L::textDim);
    g.setFont (adaptivegate::ui::Fonts::numeric (11.0f * scale));
    g.drawText ("v" JucePlugin_VersionString, versionLayoutTarget.getBounds(), juce::Justification::centredRight);
}

void AdaptiveGateAudioProcessorEditor::resized()
{
    pedalView.setBounds (getLocalBounds());

    if (currentView != ViewMode::Native)
        return;

    const float sx = (float) getWidth() / designWidth;
    const float sy = (float) getHeight() / designHeight;
    auto bounds = [sx, sy] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (juce::roundToInt (x * sx), juce::roundToInt (y * sy),
                                     juce::roundToInt (w * sx), juce::roundToInt (h * sy));
    };

    controlPanel.setBounds (getLocalBounds());
    visualizer.setBounds (bounds (25, 84, 778, 450));
    presetPanel.setBounds (bounds (650, 23, 280, 30));
    makerLayoutTarget.setBounds (bounds (42, 22, 220, 31));
    logoLayoutTarget.setBounds (bounds (527, 20, 46, 31));
    versionLayoutTarget.setBounds (bounds (938, 22, 44, 31));
    pedalViewButton.setBounds (bounds (1100.0f - 158.0f, 22.0f, 116.0f, 30.0f));

#if defined (JUCE_LAYOUT_TUNER) && JUCE_LAYOUT_TUNER
    if (layoutTuner != nullptr)
    {
        layoutTuner->setBounds (getLocalBounds());
        layoutTuner->toFront (false);
    }
#endif
}

void AdaptiveGateAudioProcessorEditor::switchToView (ViewMode mode)
{
    currentView = mode;
    const bool native = (mode == ViewMode::Native);

    controlPanel.setVisible (native);
    visualizer.setVisible (native);
    presetPanel.setVisible (native);
    pedalViewButton.setVisible (native);
    makerLayoutTarget.setVisible (native);
    logoLayoutTarget.setVisible (native);
    versionLayoutTarget.setVisible (native);
    pedalView.setVisible (! native);

    auto* constrainer = getConstrainer();

    if (native)
    {
        setResizeLimits (825, 540, 1650, 1080);
        if (constrainer != nullptr)
            constrainer->setFixedAspectRatio ((double) designWidth / (double) designHeight);
        setSize ((int) designWidth, (int) designHeight);
    }
    else
    {
        setResizeLimits (224, 336, 560, 840);
        if (constrainer != nullptr)
            constrainer->setFixedAspectRatio (1024.0 / 1536.0);
        setSize (336, 504);
    }

    resized();
    repaint();
}
