cmake_minimum_required(VERSION 3.22)

project(NkbTwin VERSION 1.3.0 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
    GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(NkbTwin
    COMPANY_NAME "NKB Audio"
    BUNDLE_ID "audio.nkb.twin"
    PLUGIN_MANUFACTURER_CODE Nkba
    PLUGIN_CODE Nktw
    PRODUCT_NAME "NKB Twin"
    DESCRIPTION "Twin Reverb-inspired clean guitar amplifier"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3 Standalone
)

juce_add_binary_data(NkbTwinBrandData
    HEADER_NAME NkbTwinBrandData.h
    NAMESPACE NkbTwinBrandData
    SOURCES Assets/nkb_twin_logo.svg
)

juce_generate_juce_header(NkbTwin)

target_sources(NkbTwin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

target_compile_features(NkbTwin PRIVATE cxx_std_17)

target_compile_definitions(NkbTwin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_ASIO=1
)

target_link_libraries(NkbTwin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        NkbTwinBrandData
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
