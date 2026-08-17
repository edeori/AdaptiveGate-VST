#include "PedalViewComponent.h"
#include "UI/AdaptiveGateFonts.h"
#include "BinaryData.h"

#include <cmath>

namespace
{
    constexpr float designWidth = 1024.0f;
    constexpr float designHeight = 1536.0f;

    // Antique-gold, matching the gate-pedal.png linework AND the knob_modern_pointer.svg's own
    // stroke colour (#d8c8ae/#efe4d1) - deliberately distinct from the native view's cyan/violet
    // "chrome" palette.
    constexpr juce::uint32 gold = 0xffd8c8ae;
    constexpr juce::uint32 goldHot = 0xffefe4d1;

    /** Reuses the MothBite plugin's own knob artwork and drawRotarySlider technique verbatim
        (Assets/knob_modern.svg + _pointer.svg, copied into this project; same DropShadow
        parameters as MothBiteLookAndFeel) - knobs sit directly on the pedal artwork with no
        extra backing panel behind them, exactly like MothBite's own pedal.png overlay. */
    class PedalKnobLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        PedalKnobLookAndFeel()
            : knobFace (juce::Drawable::createFromImageData (BinaryData::knob_modern_svg, BinaryData::knob_modern_svgSize)),
              knobPointer (juce::Drawable::createFromImageData (BinaryData::knob_modern_pointer_svg, BinaryData::knob_modern_pointer_svgSize))
        {
            setColour (juce::Slider::textBoxTextColourId, juce::Colour (gold));
            setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            setColour (juce::Label::textColourId, juce::Colour (gold));
            setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1a1710));
            setColour (juce::TextButton::textColourOffId, juce::Colour (gold));
            setColour (juce::TextButton::textColourOnId, juce::Colour (goldHot));
        }

        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float startAngle, float endAngle, juce::Slider&) override
        {
            const auto diameter = (float) juce::jmin (width, height) * 0.88f;
            const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                                     .withSizeKeepingCentre (diameter, diameter);

            juce::DropShadow (juce::Colours::black.withAlpha (0.72f), 10, { 0, 5 })
                .drawForRectangle (g, bounds.toNearestInt().reduced (6));

            if (knobFace != nullptr)
                knobFace->drawWithin (g, bounds, juce::RectanglePlacement::centred, 1.0f);

            if (knobPointer != nullptr)
            {
                const auto angle = startAngle + sliderPos * (endAngle - startAngle);
                juce::Graphics::ScopedSaveState saved (g);
                g.addTransform (juce::AffineTransform::rotation (angle, bounds.getCentreX(), bounds.getCentreY()));
                const auto fitToSvgViewBox = juce::AffineTransform::scale (bounds.getWidth() / 100.0f, bounds.getHeight() / 100.0f)
                                                 .translated (bounds.getX(), bounds.getY());
                knobPointer->draw (g, 1.0f, fitToSvgViewBox);
            }
        }

        juce::Label* createSliderTextBox (juce::Slider& slider) override
        {
            auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
            label->setFont (adaptivegate::ui::Fonts::medium (13.0f));
            label->setJustificationType (juce::Justification::centred);
            return label;
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                   bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (juce::Colour (0xff141110).withAlpha (shouldDrawButtonAsDown ? 0.95f : (shouldDrawButtonAsHighlighted ? 0.85f : 0.68f)));
            g.fillRoundedRectangle (bounds, 5.0f);
            g.setColour (juce::Colour (gold).withAlpha (shouldDrawButtonAsHighlighted ? 0.85f : 0.55f));
            g.drawRoundedRectangle (bounds, 5.0f, 1.2f);
        }

    private:
        std::unique_ptr<juce::Drawable> knobFace, knobPointer;
    };
} // namespace

PedalViewComponent::PedalViewComponent (AdaptiveGateAudioProcessor& p)
    : processorRef (p)
{
    knobLookAndFeel = std::make_unique<PedalKnobLookAndFeel>();
    setLookAndFeel (knobLookAndFeel.get());

    backgroundImage = juce::ImageFileFormat::loadFrom (BinaryData::gatepedal_png, (size_t) BinaryData::gatepedal_pngSize);

    struct KnobSpec { Knob* knob; const char* paramId; const char* label; const char* tooltip; };
    const KnobSpec specs[] = {
        { &thresholdKnob, AdaptiveGateAudioProcessor::thresholdParamId, "THRESHOLD", "How strict the gate is - higher closes on more of the signal." },
        { &sensitivityKnob, AdaptiveGateAudioProcessor::sensitivityParamId, "SENSITIVITY", "How sharply the gate snaps open/closed around the threshold." },
        { &mixKnob, AdaptiveGateAudioProcessor::mixParamId, "MIX", "Blends the dry and gated signal." },
    };

    for (const auto& spec : specs)
    {
        auto& knob = *spec.knob;
        knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 148, 24);
        knob.slider.setScrollWheelEnabled (false);
        knob.slider.setTooltip (spec.tooltip);
        addAndMakeVisible (knob.slider);
        knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            p.apvts, spec.paramId, knob.slider);

        knob.label.setText (spec.label, juce::dontSendNotification);
        knob.label.setJustificationType (juce::Justification::centred);
        knob.label.setFont (adaptivegate::ui::Fonts::medium (16.0f));
        knob.label.setColour (juce::Label::textColourId, juce::Colour (gold));
        knob.label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (knob.label);
    }

    advancedButton.setTooltip ("Switch to the full control panel.");
    advancedButton.onClick = [this] { if (onSwitchToAdvanced) onSwitchToAdvanced(); };
    addAndMakeVisible (advancedButton);

    pinSourceToGuitar();
}

PedalViewComponent::~PedalViewComponent()
{
    setLookAndFeel (nullptr);
}

void PedalViewComponent::visibilityChanged()
{
    if (isVisible())
        pinSourceToGuitar();
}

void PedalViewComponent::pinSourceToGuitar()
{
    // The pedal skin has no Source control and is explicitly a guitar-pedal aesthetic - running
    // a different profile silently behind it would be confusing, so this view always forces it.
    if (auto* param = processorRef.apvts.getParameter (AdaptiveGateAudioProcessor::sourceTypeParamId))
        param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (1.0f)); // index 1 = "Guitar"
}

void PedalViewComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff08070a));
    if (backgroundImage.isValid())
        g.drawImage (backgroundImage, getLocalBounds().toFloat());
}

void PedalViewComponent::resized()
{
    const float sx = (float) getWidth() / designWidth;
    const float sy = (float) getHeight() / designHeight;
    auto bounds = [sx, sy] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (juce::roundToInt (x * sx), juce::roundToInt (y * sy),
                                     juce::roundToInt (w * sx), juce::roundToInt (h * sy));
    };

    // Same idiom as MothBiteAudioProcessorEditor::resized(): a label directly above a rotary
    // slider whose own bounds already include its below-mounted text box, placed straight over
    // the pedal artwork - no extra backing panel. Triangle formation (Threshold apex, Sensitivity
    // + Mix at the base) aligned with the artwork's own engraved triangle/circle motif, sitting
    // in the middle of the composition rather than crowded down at the bottom edge.
    constexpr float knobWidth = 218.0f;
    constexpr float knobHeight = 258.0f;
    constexpr float labelHeight = 36.0f;

    struct Placement { float centreX, top; };
    const Placement placements[3] = {
        { 512.0f, 792.0f },  // THRESHOLD - apex
        { 300.0f, 1084.0f }, // SENSITIVITY - base left
        { 724.0f, 1084.0f }, // MIX - base right
    };

    Knob* knobs[3] = { &thresholdKnob, &sensitivityKnob, &mixKnob };
    for (int i = 0; i < 3; ++i)
    {
        const auto& p = placements[i];
        knobs[i]->label.setBounds (bounds (p.centreX - knobWidth * 0.5f - 20.0f, p.top - labelHeight + 2.0f, knobWidth + 40.0f, labelHeight));
        knobs[i]->slider.setBounds (bounds (p.centreX - knobWidth * 0.5f, p.top, knobWidth, knobHeight));
    }

    advancedButton.setBounds (bounds (designWidth - 152.0f, 40.0f, 116.0f, 46.0f));
}
