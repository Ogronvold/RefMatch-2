cmake_minimum_required(VERSION 3.22)
project(RefMatch VERSION 0.4.0 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(RefMatch
    COMPANY_NAME "RefMatch Audio"
    BUNDLE_ID "com.refmatch.audio.plugin"
    PLUGIN_MANUFACTURER_CODE RfMt
    PLUGIN_CODE RfM1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "RefMatch"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)
juce_generate_juce_header(RefMatch)

target_sources(RefMatch PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/SpectrumAnalyser.cpp
    Source/SpectrumAnalyser.h
    Source/MatchEQ.cpp
    Source/MatchEQ.h
    Source/SystemAudioCapture.mm
    Source/SystemAudioCapture.h
    Source/SpotifyDesktopController.mm
    Source/SpotifyDesktopController.h
)

target_compile_definitions(RefMatch PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(RefMatch PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(APPLE)
    find_library(SCREENCAPTUREKIT_FRAMEWORK ScreenCaptureKit REQUIRED)
    find_library(COREMEDIA_FRAMEWORK CoreMedia REQUIRED)
    find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)

    target_link_libraries(RefMatch PRIVATE
        ${SCREENCAPTUREKIT_FRAMEWORK}
        ${COREMEDIA_FRAMEWORK}
        ${COREAUDIO_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${APPKIT_FRAMEWORK}
    )

    set_source_files_properties(
        Source/SystemAudioCapture.mm
        Source/SpotifyDesktopController.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
endif()
