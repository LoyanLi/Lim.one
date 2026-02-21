cmake_minimum_required(VERSION 3.15)

project(LimOne VERSION 0.1.0)

add_subdirectory(JUCE)

juce_add_plugin(LimOne
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "LuminousLab"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Lumi
    PLUGIN_CODE LmOn
    FORMATS AU VST3 AAX Standalone
    BUNDLE_ID "com.luminouslab.limone"
    AAX_IDENTIFIER "com.luminouslab.limone"
    AAX_CATEGORY Dynamics
    AAX_COPY_DIR "${CMAKE_BINARY_DIR}/AAX_Install"
    PRODUCT_NAME "Lim.one"
)

# Add Binary Data for Web Assets
file(GLOB_RECURSE WEB_ASSET_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Resources/web/*"
)
list(FILTER WEB_ASSET_FILES EXCLUDE REGEX "\\.DS_Store$")

juce_add_binary_data(WebAssets
    SOURCES
        ${WEB_ASSET_FILES}
)

target_sources(LimOne
    PRIVATE
        Source/Plugin/PluginProcessor.cpp
        Source/Plugin/PluginProcessor.h
        Source/GUI/PluginEditor.cpp
        Source/GUI/PluginEditor.h
        Source/DSP/Clipper.h
        Source/DSP/ClipperSoftClipper.h
        Source/DSP/ClipperHardClipper.h
        Source/DSP/Limiter.h
        Source/DSP/LimiterClassic.h
        Source/DSP/LimiterModern.h
        Source/DSP/LUFSMeter/Ebu128LoudnessMeter.cpp
        Source/DSP/LUFSMeter/filters/SecondOrderIIRFilter.cpp
)

target_include_directories(LimOne
    PRIVATE
        Source/DSP
        Source/DSP/LUFSMeter
        Source/GUI
        Source/Plugin
)

# WebView2 SDK include path for Windows
if(WIN32 AND DEFINED WEBVIEW2_INCLUDE_DIR)
    target_include_directories(LimOne PRIVATE ${WEBVIEW2_INCLUDE_DIR})
    target_include_directories(juce_gui_extra INTERFACE ${WEBVIEW2_INCLUDE_DIR})
endif()

target_compile_definitions(LimOne
    PUBLIC
        JUCE_WEB_BROWSER=1
        $<$<PLATFORM_ID:Windows>:JUCE_USE_WIN_WEBVIEW2=1>
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LimOne
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        WebAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
