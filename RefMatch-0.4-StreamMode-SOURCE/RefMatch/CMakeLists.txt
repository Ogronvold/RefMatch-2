cmake_minimum_required(VERSION 3.22)
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0 CACHE STRING "Minimum macOS version")
endif()
project(RefMatch VERSION 0.4.1 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 20)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)

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

# A real Cocoa application is required: an AU's Info.plist/entitlements cannot
# grant Automation rights to its host process (Logic/AUHostingService).
add_executable(RefMatchControl MACOSX_BUNDLE Helper/main.mm)
set_target_properties(RefMatchControl PROPERTIES
    OUTPUT_NAME "RefMatch Control"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Helper/Info.plist.in"
    OBJCXX_STANDARD 20
    OBJCXX_STANDARD_REQUIRED YES
)
target_compile_options(RefMatchControl PRIVATE -fobjc-arc)
target_link_libraries(RefMatchControl PRIVATE "-framework AppKit" "-framework Carbon")

foreach(format AU VST3 Standalone)
    add_dependencies(RefMatch_${format} RefMatchControl)
    add_custom_command(TARGET RefMatch_${format} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_DIR:RefMatch_${format}>/Contents/Resources"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<TARGET_BUNDLE_DIR:RefMatchControl>"
            "$<TARGET_BUNDLE_DIR:RefMatch_${format}>/Contents/Resources/RefMatch Control.app"
        COMMAND /usr/bin/codesign --force --sign - --options runtime
            --entitlements "${CMAKE_CURRENT_SOURCE_DIR}/Helper/Entitlements.plist"
            "$<TARGET_BUNDLE_DIR:RefMatch_${format}>/Contents/Resources/RefMatch Control.app"
        COMMAND /usr/bin/codesign --force --sign -
            "$<TARGET_BUNDLE_DIR:RefMatch_${format}>"
        VERBATIM
    )
endforeach()

include(CTest)
if(BUILD_TESTING)
    add_executable(RefMatchRuntimeTests Tests/RuntimeTests.cpp)
    target_include_directories(RefMatchRuntimeTests PRIVATE Source)
    add_test(NAME RefMatchRuntime COMMAND RefMatchRuntimeTests)
endif()
