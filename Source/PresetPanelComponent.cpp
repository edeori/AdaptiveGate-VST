#include "PresetPanelComponent.h"

PresetPanelComponent::PresetPanelComponent (AdaptiveGateAudioProcessor& p)
    : processorRef (p)
{
    presetBox.setTextWhenNothingSelected ("(no preset loaded)");
    presetBox.setTooltip ("Load a previously saved preset.");
    presetBox.addListener (this);
    addAndMakeVisible (presetBox);

    saveButton.setTooltip ("Save the current control settings as a new named preset.");
    saveButton.addListener (this);
    addAndMakeVisible (saveButton);

    deleteButton.setTooltip ("Delete the currently selected preset.");
    deleteButton.addListener (this);
    addAndMakeVisible (deleteButton);

    refreshPresetList();
}

PresetPanelComponent::~PresetPanelComponent()
{
    presetBox.removeListener (this);
    saveButton.removeListener (this);
    deleteButton.removeListener (this);
}

void PresetPanelComponent::resized()
{
    auto area = getLocalBounds().reduced (4);
    deleteButton.setBounds (area.removeFromRight (70));
    area.removeFromRight (4);
    saveButton.setBounds (area.removeFromRight (70));
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
