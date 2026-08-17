#include "PresetPanelComponent.h"
#include "UI/AdaptiveGateLookAndFeel.h"

PresetPanelComponent::PresetPanelComponent (AdaptiveGateAudioProcessor& p)
    : processorRef (p)
{
    presetBox.setTextWhenNothingSelected ("(no preset loaded)");
    presetBox.setTooltip ("Load a previously saved preset.");
    presetBox.addListener (this);
    addAndMakeVisible (presetBox);

    previousButton.setTooltip ("Load the previous preset.");
    previousButton.addListener (this);
    previousButton.getProperties().set (adaptivegate::ui::LookAndFeel::chromeProperty, true);
    addAndMakeVisible (previousButton);

    nextButton.setTooltip ("Load the next preset.");
    nextButton.addListener (this);
    nextButton.getProperties().set (adaptivegate::ui::LookAndFeel::chromeProperty, true);
    addAndMakeVisible (nextButton);

    saveButton.setTooltip ("Save the current control settings as a new named preset.");
    saveButton.addListener (this);
    saveButton.getProperties().set (adaptivegate::ui::LookAndFeel::chromeProperty, true);
    addAndMakeVisible (saveButton);

    deleteButton.setTooltip ("Delete the currently selected preset.");
    deleteButton.addListener (this);
    deleteButton.getProperties().set (adaptivegate::ui::LookAndFeel::chromeProperty, true);
    addAndMakeVisible (deleteButton);

    refreshPresetList();
}

PresetPanelComponent::~PresetPanelComponent()
{
    presetBox.removeListener (this);
    previousButton.removeListener (this);
    nextButton.removeListener (this);
    saveButton.removeListener (this);
    deleteButton.removeListener (this);
}

void PresetPanelComponent::resized()
{
    auto area = getLocalBounds();
    previousButton.setBounds (area.removeFromLeft (26));
    area.removeFromLeft (4);
    deleteButton.setBounds (area.removeFromRight (46));
    area.removeFromRight (4);
    saveButton.setBounds (area.removeFromRight (50));
    area.removeFromRight (4);
    nextButton.setBounds (area.removeFromRight (26));
    area.removeFromRight (4);
    presetBox.setBounds (area);
}

void PresetPanelComponent::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);

    int itemId = 1;
    for (const auto& name : processorRef.getPresetManager().getAllPresetNames())
        presetBox.addItem (name, itemId++);
}

void PresetPanelComponent::comboBoxChanged (juce::ComboBox* box)
{
    if (box != &presetBox)
        return;

    const auto name = presetBox.getText();
    if (name.isNotEmpty())
        processorRef.getPresetManager().loadPreset (name);
}

void PresetPanelComponent::buttonClicked (juce::Button* b)
{
    if (b == &saveButton)
    {
        promptAndSavePreset();
    }
    else if (b == &previousButton)
    {
        selectRelativePreset (-1);
    }
    else if (b == &nextButton)
    {
        selectRelativePreset (1);
    }
    else if (b == &deleteButton)
    {
        const auto name = presetBox.getText();
        if (name.isNotEmpty())
        {
            processorRef.getPresetManager().deletePreset (name);
            refreshPresetList();
            presetBox.setText ({}, juce::dontSendNotification);
        }
    }
}

void PresetPanelComponent::selectRelativePreset (int delta)
{
    const int count = presetBox.getNumItems();
    if (count == 0)
        return;

    int index = presetBox.getSelectedItemIndex();
    if (index < 0)
        index = delta > 0 ? 0 : count - 1;
    else
        index = (index + delta + count) % count;
    presetBox.setSelectedItemIndex (index, juce::sendNotification);
}

void PresetPanelComponent::promptAndSavePreset()
{
    saveDialog = std::make_unique<juce::AlertWindow> ("Save Preset", "Enter a name for this preset:",
                                                       juce::MessageBoxIconType::NoIcon);
    saveDialog->addTextEditor ("name", presetBox.getText());
    saveDialog->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && saveDialog != nullptr)
        {
            const auto name = saveDialog->getTextEditorContents ("name").trim();
            if (name.isNotEmpty())
            {
                processorRef.getPresetManager().savePreset (name);
                refreshPresetList();
                presetBox.setText (name, juce::dontSendNotification);
            }
        }
        saveDialog.reset();
    }));
}
