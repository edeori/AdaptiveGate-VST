cmake_minimum_required(VERSION 3.22)

project(AdaptiveGate VERSION 0.1.10)

option(ADAPTIVEGATE_LAYOUT_TUNER "Enable the debug-only JUCE layout tuner" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(AdaptiveGateAssets SOURCES
    Assets/background.png
    Assets/moth_logo.png
    Assets/SplitFire-knob.png
    Assets/SplitFire-rect-off.png
    Assets/SplitFire-rect-on.png
    Assets/StringLife-slider_thumb.png
    Assets/gate-pedal.png
    Assets/knob_modern.svg
    Assets/knob_modern_pointer.svg
    Assets/Fonts/Oxanium-Bold.ttf
    Assets/Fonts/BarlowCondensed-Light.ttf
    Assets/Fonts/BarlowCondensed-Medium.ttf
    Assets/Fonts/Rajdhani-Medium.ttf
)

juce_add_plugin(AdaptiveGate
    COMPANY_NAME "Moth Production"
    BUNDLE_ID "com.mothproduction.adaptivegate"
    PLUGIN_MANUFACTURER_CODE Moth
    PLUGIN_CODE Agt1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "AdaptiveGate"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(AdaptiveGate PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/GateVisualizerComponent.cpp
    Source/PresetManager.cpp
    Source/PresetPanelComponent.cpp
    Source/ControlPanelComponent.cpp
    Source/UI/AdaptiveGateFonts.cpp
    Source/UI/AdaptiveGateLookAndFeel.cpp
    Source/UI/PedalViewComponent.cpp
    Source/DSP/FilterBank.cpp
    Source/DSP/EnvelopeFollower.cpp
    Source/DSP/NoiseFloorTracker.cpp
    Source/DSP/TransientDetector.cpp
    Source/DSP/GateDecision.cpp
    Source/DSP/GainEnvelope.cpp
    Source/DSP/AdaptiveGateEngine.cpp
    Source/Presets/FrequencyProfile.cpp
    Source/Presets/Coherence.cpp
)

target_include_directories(AdaptiveGate PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(AdaptiveGate
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

if(ADAPTIVEGATE_LAYOUT_TUNER)
    target_compile_definitions(AdaptiveGate PRIVATE JUCE_LAYOUT_TUNER=1)
endif()

target_link_libraries(AdaptiveGate
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        AdaptiveGateAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
