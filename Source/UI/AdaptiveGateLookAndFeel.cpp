#include "AdaptiveGateLookAndFeel.h"
#include "AdaptiveGateFonts.h"
#include "BinaryData.h"

namespace adaptivegate::ui
{
    const juce::Colour LookAndFeel::background { 0xff050609 };
    const juce::Colour LookAndFeel::panel      { 0xff0b0d12 };
    const juce::Colour LookAndFeel::cyan       { 0xff42d9ff };
    const juce::Colour LookAndFeel::cyanHot    { 0xff96edff };
    const juce::Colour LookAndFeel::violet     { 0xff944cff };
    const juce::Colour LookAndFeel::violetHot  { 0xffc190ff };
    const juce::Colour LookAndFeel::text       { 0xffe9ecf3 };
    const juce::Colour LookAndFeel::textDim    { 0xff79818e };
    const juce::Colour LookAndFeel::threshold  { 0xffffb521 };
    const juce::Identifier LookAndFeel::chromeProperty { "adaptiveGateChrome" };

    namespace
    {
        const juce::Rectangle<int> chromeSource { 40, 45, 520, 130 };

        void drawChrome (juce::Graphics& g, const juce::Image& image, juce::Rectangle<float> area)
        {
            if (! image.isValid())
                return;
            juce::Path clip;
            clip.addRoundedRectangle (area, area.getHeight() * 0.2f);
            g.saveState();
            g.reduceClipRegion (clip);
            g.drawImage (image, (int) area.getX(), (int) area.getY(), (int) area.getWidth(), (int) area.getHeight(),
                         chromeSource.getX(), chromeSource.getY(), chromeSource.getWidth(), chromeSource.getHeight());
            g.restoreState();
        }
    }

    LookAndFeel::LookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, background);
        setColour (juce::Label::textColourId, text);
        setColour (juce::Slider::textBoxTextColourId, text);
        setColour (juce::Slider::textBoxBackgroundColourId, background.withAlpha (0.88f));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0d0f14));
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, cyan.withAlpha (0.24f));
        setColour (juce::PopupMenu::highlightedTextColourId, text);
        setColour (juce::TextButton::textColourOffId, textDim);
        setColour (juce::TextButton::textColourOnId, cyanHot);

        knob = juce::ImageFileFormat::loadFrom (BinaryData::SplitFireknob_png, (size_t) BinaryData::SplitFireknob_pngSize);
        rectOff = juce::ImageFileFormat::loadFrom (BinaryData::SplitFirerectoff_png, (size_t) BinaryData::SplitFirerectoff_pngSize);
        rectOn = juce::ImageFileFormat::loadFrom (BinaryData::SplitFirerecton_png, (size_t) BinaryData::SplitFirerecton_pngSize);
        sliderThumb = juce::ImageFileFormat::loadFrom (BinaryData::StringLifeslider_thumb_png, (size_t) BinaryData::StringLifeslider_thumb_pngSize);
    }

    void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float position, float start, float end, juce::Slider& slider)
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const auto centre = bounds.getCentre();
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float faceRadius = radius * 0.82f;
        const float angle = start + position * (end - start);
        const auto face = juce::Rectangle<float> (faceRadius * 2.0f, faceRadius * 2.0f).withCentre (centre);

        for (int i = 0; i < 21; ++i)
        {
            const float tick = start + (float) i / 20.0f * (end - start);
            const juce::Point<float> a { centre.x + radius * 0.90f * std::sin (tick), centre.y - radius * 0.90f * std::cos (tick) };
            const juce::Point<float> b { centre.x + radius * std::sin (tick), centre.y - radius * std::cos (tick) };
            g.setColour (tick <= angle + 0.001f && slider.isEnabled() ? cyan : juce::Colour (0xff343943));
            g.drawLine ({ a, b }, 1.25f);
        }

        if (knob.isValid())
        {
            g.saveState();
            juce::Path clip; clip.addEllipse (face.reduced (face.getWidth() * 0.02f));
            g.reduceClipRegion (clip);
            g.addTransform (juce::AffineTransform::rotation (angle - (start + end) * 0.5f, centre.x, centre.y));
            g.setOpacity (slider.isEnabled() ? 1.0f : 0.45f);
            g.drawImage (knob, face);
            g.restoreState();
        }
    }

    void LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float, float, juce::Slider::SliderStyle style, juce::Slider& slider)
    {
        const bool vertical = style == juce::Slider::LinearVertical;
        const float thickness = 5.0f;
        auto track = vertical ? juce::Rectangle<float> ((float) x + width * 0.5f - thickness * 0.5f, (float) y, thickness, (float) height)
                              : juce::Rectangle<float> ((float) x, (float) y + height * 0.5f - thickness * 0.5f, (float) width, thickness);
        g.setColour (juce::Colour (0xff151821));
        g.fillRoundedRectangle (track, 2.5f);
        auto fill = vertical ? track.withTop (sliderPos) : track.withRight (sliderPos);
        g.setColour (slider.isEnabled() ? cyan : textDim);
        g.fillRoundedRectangle (fill, 2.5f);

        const float size = juce::jmin (27.0f, vertical ? (float) width * 0.62f : (float) height * 0.72f);
        auto thumb = juce::Rectangle<float> (size, size);
        thumb.setCentre (vertical ? track.getCentreX() : sliderPos, vertical ? sliderPos : track.getCentreY());
        if (sliderThumb.isValid())
            g.drawImage (sliderThumb, thumb);
        else
        {
            g.setColour (cyanHot);
            g.fillEllipse (thumb);
        }
    }

    void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool)
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto track = juce::Rectangle<float> (34.0f, 18.0f).withCentre (bounds.getCentre());
        g.setColour (background); g.fillRoundedRectangle (track, 9.0f);
        g.setColour (button.getToggleState() ? cyan.withAlpha (0.8f) : juce::Colour (0xff303640));
        g.drawRoundedRectangle (track, 9.0f, 1.0f);
        const float x = button.getToggleState() ? track.getRight() - 14.0f : track.getX() + 3.0f;
        auto dot = juce::Rectangle<float> (11.0f, 11.0f).withPosition (x, track.getY() + 3.5f);
        g.setColour (button.getToggleState() ? cyanHot : juce::Colour (0xff555d68)); g.fillEllipse (dot);
        if (highlighted) { g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (track, 9.0f); }
    }

    void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool highlighted, bool down)
    {
        auto area = button.getLocalBounds().toFloat().reduced (1.0f);
        const auto& image = button.getToggleState() ? rectOn : rectOff;
        if ((bool) button.getProperties()[chromeProperty] && image.isValid())
            drawChrome (g, image, area);
        else { g.setColour (juce::Colour (0xff08090c)); g.fillRoundedRectangle (area, 4.0f); }
        if (highlighted || down) { g.setColour (juce::Colours::white.withAlpha (down ? 0.1f : 0.05f)); g.fillRoundedRectangle (area, 4.0f); }
        g.setColour ((button.getToggleState() ? cyanHot : cyan).withAlpha (button.getToggleState() ? 0.9f : 0.48f));
        g.drawRoundedRectangle (area, 4.0f, button.getToggleState() ? 1.5f : 1.0f);
    }

    void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
    {
        g.setColour (button.getToggleState() ? cyanHot : textDim);
        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
    }

    void LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool down, int, int, int, int, juce::ComboBox& box)
    {
        auto area = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
        if (rectOff.isValid()) drawChrome (g, rectOff, area);
        else { g.setColour (panel); g.fillRoundedRectangle (area, 4.0f); }
        g.setColour (cyanHot.withAlpha (box.isEnabled() ? (down ? 0.95f : 0.68f) : 0.3f));
        g.drawRoundedRectangle (area, 4.0f, 1.1f);
        auto c = juce::Point<float> ((float) width - 10.0f, (float) height * 0.5f);
        juce::Path arrow; arrow.startNewSubPath (c.x - 3.0f, c.y - 1.5f); arrow.lineTo (c.x, c.y + 1.7f); arrow.lineTo (c.x + 3.0f, c.y - 1.5f);
        g.setColour (cyanHot); g.strokePath (arrow, juce::PathStrokeType (1.2f));
    }

    void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        label.setBounds (7, 1, box.getWidth() - 22, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return Fonts::medium ((float) h * 0.43f); }
    juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox& box) { return Fonts::medium ((float) box.getHeight() * 0.43f); }
    juce::Font LookAndFeel::getPopupMenuFont() { return Fonts::light (15.0f); }
}
