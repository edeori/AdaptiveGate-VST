#include "PresetManager.h"

namespace adaptivegate
{

namespace
{
    constexpr const char* presetFileExtension = ".xml";
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& stateToManage)
    : apvts (stateToManage)
{
    getPresetsDirectory().createDirectory();
}

juce::File PresetManager::getPresetsDirectory()
{
    // userApplicationDataDirectory resolves to ~/Library (not ~/Library/Application
    // Support) on macOS - "Application Support" has to be appended explicitly to land
    // next to this plugin's sibling Moth Production products.
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Application Support")
        .getChildFile ("Moth Production")
        .getChildFile ("AdaptiveGate")
        .getChildFile ("Presets");
}

void PresetManager::savePreset (const juce::String& presetName)
{
    if (presetName.isEmpty())
        return;

    const auto state = apvts.copyState();
    const std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr)
        return;

    xml->writeTo (getPresetsDirectory().getChildFile (presetName + presetFileExtension));
}

void PresetManager::loadPreset (const juce::String& presetName)
{
    const auto file = getPresetsDirectory().getChildFile (presetName + presetFileExtension);
    if (! file.existsAsFile())
        return;

    const std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

void PresetManager::deletePreset (const juce::String& presetName)
{
    getPresetsDirectory().getChildFile (presetName + presetFileExtension).deleteFile();
}

juce::StringArray PresetManager::getAllPresetNames() const
{
    juce::StringArray names;

    for (const auto& entry : juce::RangedDirectoryIterator (getPresetsDirectory(), false, juce::String ("*") + presetFileExtension))
        names.add (entry.getFile().getFileNameWithoutExtension());

    names.sort (true);
    return names;
}

} // namespace adaptivegate
