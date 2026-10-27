cmake_minimum_required(VERSION 3.22)
project(Thudi VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(THUDI_BUILD_PLUGIN "Build the VST3 plugin" ON)
option(THUDI_BUILD_TOOLS  "Build offline tools (renderer, snapshot, logo)" ON)
set(THUDI_JUCE_DIR "" CACHE PATH "Use a local JUCE checkout instead of downloading")

# ---------------------------------------------------------------- DSP core (no JUCE)
add_library(thudi_dsp STATIC
    src/dsp/Voice.cpp
    src/dsp/PadChain.cpp
    src/dsp/Engine.cpp
    src/dsp/SampleData.cpp)
target_include_directories(thudi_dsp PUBLIC src/dsp)
if(MSVC)
    target_compile_options(thudi_dsp PRIVATE /W3 /fp:fast)
else()
    target_compile_options(thudi_dsp PRIVATE -Wall -Wextra -Wno-unused-parameter -O2)
endif()

if(THUDI_BUILD_TOOLS)
    add_executable(thudi_render tools/thudi_render.cpp)
    target_link_libraries(thudi_render PRIVATE thudi_dsp)
    # engine checks that have no reference recording (empty pads, pad glow, solo mode, sample chops, GÆRRO noise floor)
    add_executable(thudi_dsp_tests tools/thudi_dsp_tests.cpp)
    target_link_libraries(thudi_dsp_tests PRIVATE thudi_dsp)
endif()

if(NOT THUDI_BUILD_PLUGIN AND NOT THUDI_BUILD_TOOLS)
    return()
endif()

# ---------------------------------------------------------------- JUCE
include(FetchContent)
if(THUDI_JUCE_DIR)
    add_subdirectory(${THUDI_JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()

# Linux build flags (CI snapshot / local checks): avoid optional X11 extensions and network libs
set(THUDI_LINUX_DEFS)
if(UNIX AND NOT APPLE)
    set(THUDI_LINUX_DEFS JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0)
endif()

# ---------------------------------------------------------------- embedded resources
file(GLOB THUDI_UI_ASSETS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/*.png
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/*.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/logo/*.path
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaerro/*.png
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaerro/*.json
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/factory/kits/*.thudikit)
file(GLOB_RECURSE THUDI_PAD_PRESETS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/presets/factory/pads/*.thudipad)
juce_add_binary_data(ThudiData NAMESPACE ThudiBinary SOURCES ${THUDI_UI_ASSETS} ${THUDI_PAD_PRESETS})

set(THUDI_PLUGIN_SOURCES
    src/plugin/PluginProcessor.cpp
    src/plugin/PresetManager.cpp
    src/plugin/Assets.cpp
    src/plugin/ThudiLookAndFeel.cpp
    src/plugin/PadGrid.cpp
    src/plugin/LcdDisplay.cpp
    src/plugin/GaerroArt.cpp
    src/plugin/PluginEditor.cpp)

set(THUDI_JUCE_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1
    ${THUDI_LINUX_DEFS})

if(THUDI_BUILD_PLUGIN)
    juce_add_plugin(Thudi
        COMPANY_NAME "R4N"
        COMPANY_WEBSITE ""
        BUNDLE_ID com.r4n.thudi
        PLUGIN_MANUFACTURER_CODE R4Nw
        PLUGIN_CODE Thdi
        FORMATS VST3
        PRODUCT_NAME "Thudi"
        DESCRIPTION "Thudi - drum synthesizer by R4N"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        VST3_CATEGORIES Instrument Drum Synth
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(Thudi PRIVATE ${THUDI_PLUGIN_SOURCES})
    target_include_directories(Thudi PRIVATE src src/plugin src/dsp)
    target_compile_definitions(Thudi PUBLIC ${THUDI_JUCE_DEFS})
    target_link_libraries(Thudi
        PRIVATE thudi_dsp ThudiData
                juce::juce_audio_processors juce::juce_audio_formats juce::juce_gui_basics juce::juce_graphics
        PUBLIC  juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endif()

if(THUDI_BUILD_TOOLS)
    # Headless test: builds the processor + editor, loads factory kits, exercises pad actions and saves GUI screenshots
    juce_add_console_app(thudi_snapshot PRODUCT_NAME "thudi_snapshot")
    target_sources(thudi_snapshot PRIVATE tools/thudi_snapshot.cpp ${THUDI_PLUGIN_SOURCES})
    target_include_directories(thudi_snapshot PRIVATE src src/plugin src/dsp)
    target_compile_definitions(thudi_snapshot PRIVATE ${THUDI_JUCE_DEFS} JucePlugin_Build_Standalone=0 THUDI_HEADLESS=1)
    target_link_libraries(thudi_snapshot PRIVATE thudi_dsp ThudiData
        juce::juce_audio_processors juce::juce_gui_basics juce::juce_graphics juce::juce_audio_formats
        juce::juce_recommended_config_flags)

    # Converts the Tamil word துடி from Kavivanar into a vector path (assets/logo/thudi_tamil.svg)
    juce_add_console_app(thudi_logo PRODUCT_NAME "thudi_logo")
    target_sources(thudi_logo PRIVATE tools/thudi_logo.cpp)
    target_compile_definitions(thudi_logo PRIVATE ${THUDI_JUCE_DEFS})
    target_link_libraries(thudi_logo PRIVATE juce::juce_graphics juce::juce_recommended_config_flags)
endif()
