cmake_minimum_required(VERSION 3.22)
project(DAroom VERSION 1.2.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# The editable factory presets are embedded as binary data so the plug-in and
# the files in Presets/ can never drift apart.
file(GLOB DAROOM_PRESET_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Presets/*.drpreset)

juce_add_binary_data(DAroomAssets
    SOURCES
        Assets/DAroom_UI_Reference.png
        ${DAROOM_PRESET_FILES}
)

juce_add_plugin(DAroom
    COMPANY_NAME "DAroom Audio"
    BUNDLE_ID "audio.daroom.reverb"
    PLUGIN_MANUFACTURER_CODE DrAm
    PLUGIN_CODE DArm
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "DAroom"
    DESCRIPTION "DAroom algorithmic reverb"
    VST3_CATEGORIES Fx Reverb
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(DAroom PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

# Generates JuceLibraryCode/JuceHeader.h (missing in the original project).
juce_generate_juce_header(DAroom)

target_compile_definitions(DAroom PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DAroom PRIVATE
    DAroomAssets
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
